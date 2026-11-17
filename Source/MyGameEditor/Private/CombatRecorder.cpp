#include "CombatRecorder.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayCueManager.h"
#include "GameplayCueNotify_Actor.h"
#include "GameplayEffect.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "AssHitBox.h"
#include "Player/MyProjectile.h"
#include "Abilities/MyAttributeSet.h"
#include "AssTestProbe.h"

UCombatRecorder* UCombatRecorder::Current = nullptr;

namespace
{
	TSharedPtr<FJsonValue> Num(double Value) { return MakeShared<FJsonValueNumber>(Value); }
	TSharedPtr<FJsonValue> Str(const FString& Value) { return MakeShared<FJsonValueString>(Value); }
	TSharedPtr<FJsonValue> Vec(const FVector& V) { return MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{Num(V.X), Num(V.Y), Num(V.Z)}); }

	// an asset's package path for a Blueprint class (/Game/Abilities/GA_Punch), the class path for a native one
	FString ClassId(const UClass* Class)
	{
		if (!Class) return FString();
		return Class->IsNative() ? Class->GetPathName() : Class->GetOutermost()->GetName();
	}

	FString ObjectId(const UObject* Object) { return Object ? ClassId(Object->GetClass()) : FString(); }
}

bool UCombatRecorder::Start(UWorld* World, const TArray<FString>& RoleNames, const TArray<AActor*>& Actors)
{
	if (!World || RoleNames.Num() != Actors.Num()) return false;
	if (Current) Stop();
	Current = NewObject<UCombatRecorder>();
	Current->AddToRoot();
	Current->Begin(World);
	for (int32 i = 0; i < RoleNames.Num(); ++i)
	{
		if (Actors[i]) Current->AddRole(RoleNames[i], Actors[i]);
	}
	return true;
}

void UCombatRecorder::QueryRoles(const TArray<FString>& Names)
{
	if (!Current) return;
	for (FRole& Role : Current->Roles) Role.bQuery = Names.Contains(Role.Name);
}

bool UCombatRecorder::IsRecording()
{
	return Current != nullptr;
}

void UCombatRecorder::Begin(UWorld* InWorld)
{
	World = InWorld;
	RoleClasses = MakeShared<FJsonObject>();
	TickStartHandle = FWorldDelegates::OnWorldTickStart.AddUObject(this, &UCombatRecorder::OnTickStart);
	TickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UCombatRecorder::OnPostActorTick);
	SpawnHandle = InWorld->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UCombatRecorder::OnSpawned));
	if (UGameplayCueManager* Cues = UAbilitySystemGlobals::Get().GetGameplayCueManager())
	{
		CueHandle = Cues->OnGameplayCueRouted().AddUObject(this, &UCombatRecorder::OnCue);
	}
	ProbeHandle = FAssTestProbe::OnEvent.AddUObject(this, &UCombatRecorder::OnProbe);
}

double UCombatRecorder::Now() const
{
	return Elapsed;
}

void UCombatRecorder::OnTickStart(UWorld* InWorld, ELevelTick TickType, float DeltaSeconds)
{
	if (InWorld != World.Get()) return;
	++Frame;
	Elapsed += DeltaSeconds;
}

void UCombatRecorder::AddRole(const FString& Name, AActor* Actor)
{
	const int32 Index = Roles.Num();
	FRole& Role = Roles.AddDefaulted_GetRef();
	Role.Name = Name;
	Role.Actor = Actor;
	Names.Add(Actor, Name);
	RoleClasses->SetStringField(Name, ObjectId(Actor));
	TickAfterEarlierRoles(Actor);
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
	if (!ASC) return;
	Role.ASC = ASC;
	Role.Applied = ASC->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UCombatRecorder::OnEffect, Index, false);
	Role.Periodic = ASC->OnPeriodicGameplayEffectExecuteDelegateOnSelf.AddUObject(this, &UCombatRecorder::OnEffect, Index, true);
	Role.Removed = ASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UCombatRecorder::OnEffectRemoved, Index);
	Role.Tags = ASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UCombatRecorder::OnTag, Index);
	Role.Activated = ASC->AbilityActivatedCallbacks.AddUObject(this, &UCombatRecorder::OnActivated, Index);
	Role.Ended = ASC->OnAbilityEnded.AddUObject(this, &UCombatRecorder::OnEnded, Index);
	Role.Failed = ASC->AbilityFailedCallbacks.AddUObject(this, &UCombatRecorder::OnFailed, Index);
	for (TFieldIterator<FProperty> It(UMyAttributeSet::StaticClass()); It; ++It)
	{
		if (!FGameplayAttribute::IsGameplayAttributeDataProperty(*It)) continue;
		const FGameplayAttribute Attribute(*It);
		Role.AttributeHandles.Emplace(Attribute, ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &UCombatRecorder::OnAttribute, Index));
	}
}

namespace
{
	void AddTickFunctions(AActor* Actor, TArray<TPair<UObject*, FTickFunction*>>& Out)
	{
		if (Actor->PrimaryActorTick.bCanEverTick) Out.Emplace(Actor, &Actor->PrimaryActorTick);
		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (Component && Component->PrimaryComponentTick.bCanEverTick) Out.Emplace(Component, &Component->PrimaryComponentTick);
		}
	}

	// A role's tick functions, its controller's included (an AI's tree presses its buttons), in one fixed order: the
	// engine's own prerequisites first (a mesh after its movement), then the controller's side, the actor before its
	// components, and names without their instance numbers
	TArray<TPair<UObject*, FTickFunction*>> TickFunctionsOf(AActor* Actor)
	{
		TArray<TPair<UObject*, FTickFunction*>> All;
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			if (AController* Controller = Pawn->GetController()) AddTickFunctions(Controller, All);
		}
		AddTickFunctions(Actor, All);
		auto Key = [Actor](const TPair<UObject*, FTickFunction*>& F)
		{
			const AActor* Owner = Cast<AActor>(F.Key) ? Cast<AActor>(F.Key) : F.Key->GetTypedOuter<AActor>();
			return FString::Printf(TEXT("%d%d%s"), Owner == Actor ? 1 : 0, F.Key->IsA<AActor>() ? 0 : 1, *F.Key->GetFName().GetPlainNameString());
		};
		All.Sort([&Key](const TPair<UObject*, FTickFunction*>& A, const TPair<UObject*, FTickFunction*>& B) { return Key(A) < Key(B); });
		// topological: take the first in key order whose prerequisites among these are all taken
		TArray<TPair<UObject*, FTickFunction*>> Sorted;
		while (All.Num())
		{
			int32 Next = 0;
			for (int32 i = 0; i < All.Num(); ++i)
			{
				const bool bReady = !All[i].Value->GetPrerequisites().ContainsByPredicate([&All](const FTickPrerequisite& Pre)
				{
					return All.ContainsByPredicate([&Pre](const TPair<UObject*, FTickFunction*>& F) { return F.Value == Pre.PrerequisiteTickFunction; });
				});
				if (bReady) { Next = i; break; }
			}
			Sorted.Add(All[Next]);
			All.RemoveAt(Next);
		}
		return Sorted;
	}
}

void UCombatRecorder::TickAfterEarlierRoles(AActor* Actor)
{
	const TArray<TPair<UObject*, FTickFunction*>> Mine = TickFunctionsOf(Actor);
	TArray<TPair<UObject*, FTickFunction*>> Before;
	for (const FRole& Earlier : Roles)
	{
		AActor* Other = Earlier.Actor.Get();
		if (Other && Other != Actor) Before.Append(TickFunctionsOf(Other));
	}
	auto Order = [this](const TPair<UObject*, FTickFunction*>& After, const TPair<UObject*, FTickFunction*>& First)
	{
		// only inside one group: a prerequisite from a later group would move this tick to that group
		if (First.Value->TickGroup != After.Value->TickGroup) return;
		After.Value->AddPrerequisite(First.Key, *First.Value);
		Orders.Add({After.Key, After.Value, First.Key, First.Value});
	};
	for (int32 i = 0; i < Mine.Num(); ++i)
	{
		for (const TPair<UObject*, FTickFunction*>& First : Before) Order(Mine[i], First);
		// the role's own functions, each after the one before it in its group
		for (int32 j = i - 1; j >= 0; --j)
		{
			if (Mine[j].Value->TickGroup == Mine[i].Value->TickGroup) { Order(Mine[i], Mine[j]); break; }
		}
	}
}

void UCombatRecorder::Unbind(FRole& Role)
{
	UAbilitySystemComponent* ASC = Role.ASC.Get();
	if (!ASC) return;
	ASC->OnGameplayEffectAppliedDelegateToSelf.Remove(Role.Applied);
	ASC->OnPeriodicGameplayEffectExecuteDelegateOnSelf.Remove(Role.Periodic);
	ASC->OnAnyGameplayEffectRemovedDelegate().Remove(Role.Removed);
	ASC->RegisterGenericGameplayTagEvent().Remove(Role.Tags);
	ASC->AbilityActivatedCallbacks.Remove(Role.Activated);
	ASC->OnAbilityEnded.Remove(Role.Ended);
	ASC->AbilityFailedCallbacks.Remove(Role.Failed);
	for (const TPair<FGameplayAttribute, FDelegateHandle>& Pair : Role.AttributeHandles)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(Pair.Key).Remove(Pair.Value);
	}
}

FString UCombatRecorder::Stop()
{
	UCombatRecorder* Self = Current;
	if (!Self) return FString();
	Current = nullptr;
	FWorldDelegates::OnWorldTickStart.Remove(Self->TickStartHandle);
	FWorldDelegates::OnWorldPostActorTick.Remove(Self->TickHandle);
	if (UWorld* W = Self->World.Get()) W->RemoveOnActorSpawnedHandler(Self->SpawnHandle);
	if (UGameplayCueManager* Cues = UAbilitySystemGlobals::Get().GetGameplayCueManager()) Cues->OnGameplayCueRouted().Remove(Self->CueHandle);
	FAssTestProbe::OnEvent.Remove(Self->ProbeHandle);
	for (const FOrder& Order : Self->Orders)
	{
		if (Order.AfterOwner.IsValid() && Order.FirstOwner.IsValid()) Order.After->RemovePrerequisite(Order.FirstOwner.Get(), *Order.First);
	}
	TSharedRef<FJsonObject> Frames = MakeShared<FJsonObject>();
	for (FRole& Role : Self->Roles)
	{
		Self->Unbind(Role);
		Frames->SetArrayField(Role.Name, Role.Frames);
	}
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetObjectField(TEXT("roles"), Self->RoleClasses);
	Root->SetObjectField(TEXT("frames"), Frames);
	Root->SetArrayField(TEXT("events"), Self->Events);
	Root->SetNumberField(TEXT("frame_count"), Self->Frame);
	FString Out;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	Self->RemoveFromRoot();
	return Out;
}

FString UCombatRecorder::RoleOf(const UObject* Object)
{
	if (!Object) return FString();
	if (const FString* Name = Names.Find(Object)) return *Name;
	// an actor nobody named (a projectile, a summon, a hitbox): its class and the order it first showed up in
	const UObject* Named = Object->IsA<AActor>() ? Object : Object->GetTypedOuter<AActor>();
	if (Named && Named != Object)
	{
		if (const FString* Name = Names.Find(Named)) return *Name;
	}
	if (!Named) Named = Object;
	const FString ClassName = Named->GetClass()->GetName();
	const FString Name = FString::Printf(TEXT("%s#%d"), *ClassName, ++ClassCounts.FindOrAdd(ClassName));
	Names.Add(Named, Name);
	return Name;
}

void UCombatRecorder::Event(const FString& Kind, const FString& Role, TArray<TSharedPtr<FJsonValue>> Fields)
{
	TArray<TSharedPtr<FJsonValue>> Row{Num(Frame), Num(Now()), Str(Kind), Str(Role)};
	Row.Append(MoveTemp(Fields));
	Events.Add(MakeShared<FJsonValueArray>(Row));
}

void UCombatRecorder::OnPostActorTick(UWorld* InWorld, ELevelTick TickType, float DeltaSeconds)
{
	if (InWorld != World.Get()) return;
	const double T = Now();

	for (FRole& Role : Roles)
	{
		AActor* Actor = Role.Actor.Get();
		if (!Actor) continue;
		const FVector L = Actor->GetActorLocation();
		const FRotator R = Actor->GetActorRotation();
		FString Montage, Section;
		double Position = -1.0;
		int32 Mode = -1;
		const ACharacter* Char = Cast<ACharacter>(Actor);
		UAnimInstance* Anim = Char && Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
		if (Anim)
		{
			if (const UAnimMontage* Active = Anim->GetCurrentActiveMontage())
			{
				Montage = Active->GetOutermost()->GetName();
				Section = Anim->Montage_GetCurrentSection(Active).ToString();
				Position = Anim->Montage_GetPosition(Active);
			}
			const UAnimMontage* Active = Anim->GetCurrentActiveMontage();
			const float At = Active ? Anim->Montage_GetPosition(Active) : -1.f;
			TSet<FString> States;
			TMap<FString, TPair<FString, FString>> Labels;
			auto AddState = [&States, &Labels](const FAnimNotifyEvent& State)
			{
				const FString Key = FString::Printf(TEXT("%s|%s|%.6f"), *State.NotifyName.ToString(), *ObjectId(State.NotifyStateClass), State.GetTriggerTime());
				States.Add(Key);
				Labels.Add(Key, {State.NotifyName.ToString(), ObjectId(State.NotifyStateClass)});
			};
			// the notifies of this frame's pose tick; a mesh that didn't tick this frame still holds the last ones (a mesh
			// that only ticks its montages, when not rendered, doesn't move its graph's update counter, so that can't tell)
			const bool bTicked = Char->GetMesh()->PoseTickedThisFrame();
			if (bTicked)
			{
				for (const FAnimNotifyEventReference& Ref : Anim->NotifyQueue.AnimNotifies)
				{
					const FAnimNotifyEvent* Notify = Ref.GetNotify();
					if (Notify && Notify->Notify)
					{
						Event(TEXT("anim"), Role.Name, {Str(TEXT("fired")), Str(Notify->NotifyName.ToString()), Str(ObjectId(Notify->Notify))});
					}
				}
				for (const FAnimNotifyEvent& State : Anim->ActiveAnimNotifyState) AddState(State);
			}
			// the montage's branching points, from its position as the engine takes them
			if (Active)
			{
				const bool bSame = Role.LastMontage.Get() == Active;
				const float Before = bSame ? Role.LastPosition : -1.f;
				for (const FAnimNotifyEvent& Notify : Active->Notifies)
				{
					if (Notify.MontageTickType != EMontageNotifyTickType::BranchingPoint) continue;
					if (Notify.NotifyStateClass)
					{
						if (At >= Notify.GetTriggerTime() && At < Notify.GetEndTriggerTime()) AddState(Notify);
					}
					else if (Notify.Notify && At >= Notify.GetTriggerTime() && Before < Notify.GetTriggerTime())
					{
						Event(TEXT("anim"), Role.Name, {Str(TEXT("fired")), Str(Notify.NotifyName.ToString()), Str(ObjectId(Notify.Notify))});
					}
				}
			}
			Role.LastMontage = Active;
			Role.LastPosition = At;
			if (!bTicked)
			{
				// keep the queue's states of the last tick
				for (const FString& Key : Role.ActiveStates)
				{
					if (!States.Contains(Key) && !Key.IsEmpty()) { TArray<FString> P; Key.ParseIntoArray(P, TEXT("|")); States.Add(Key); Labels.Add(Key, {P[0], P.Num() > 1 ? P[1] : FString()}); }
				}
			}
			for (const FString& Key : Role.ActiveStates)
			{
				if (!States.Contains(Key))
				{
					TArray<FString> Parts;
					Key.ParseIntoArray(Parts, TEXT("|"));
					Event(TEXT("anim"), Role.Name, {Str(TEXT("end")), Str(Parts[0]), Str(Parts.Num() > 1 ? Parts[1] : FString())});
				}
			}
			for (const FString& Key : States)
			{
				if (!Role.ActiveStates.Contains(Key)) Event(TEXT("anim"), Role.Name, {Str(TEXT("begin")), Str(Labels[Key].Key), Str(Labels[Key].Value)});
			}
			Role.ActiveStates = MoveTemp(States);
		}
		if (Char && Char->GetCharacterMovement()) Mode = Char->GetCharacterMovement()->MovementMode;
		const AAIController* AI = Char ? Cast<AAIController>(Char->GetController()) : nullptr;
		const UBehaviorTreeComponent* Tree = AI ? Cast<UBehaviorTreeComponent>(AI->GetBrainComponent()) : nullptr;
		const FString Tasks = Tree && Tree->IsRunning() ? Tree->DescribeActiveTasks() : FString();
		if (Tasks != Role.LastTasks)
		{
			Event(TEXT("bt"), Role.Name, {Str(Tasks)});
			Role.LastTasks = Tasks;
		}
		// the attack token: the character's aggro list (a property MyGame keeps on the character)
		if (const FArrayProperty* List = CastField<FArrayProperty>(Actor->GetClass()->FindPropertyByName(TEXT("AggroList"))))
		{
			FScriptArrayHelper Items(List, List->ContainerPtrToValuePtr<void>(Actor));
			const FObjectPropertyBase* Inner = CastField<FObjectPropertyBase>(List->Inner);
			TArray<TSharedPtr<FJsonValue>> Holders;
			FString Key;
			for (int32 i = 0; Inner && i < Items.Num(); ++i)
			{
				const FString Holder = RoleOf(Inner->GetObjectPropertyValue(Items.GetRawPtr(i)));
				Holders.Add(Str(Holder));
				Key += Holder + TEXT("|");
			}
			if (Key != Role.LastToken)
			{
				Event(TEXT("token"), Role.Name, {MakeShared<FJsonValueArray>(Holders)});
				Role.LastToken = Key;
			}
		}
		TArray<TSharedPtr<FJsonValue>> Row{
			Num(Frame), Num(T), Num(L.X), Num(L.Y), Num(L.Z), Num(R.Pitch), Num(R.Yaw), Num(R.Roll),
			Str(Montage), Str(Section), Num(Position), Num(Actor->CustomTimeDilation), Num(Mode)};
		UAbilitySystemComponent* ASC = Role.ASC.Get();
		if (Role.bQuery && ASC && ASC->AbilityActorInfo.IsValid())
		{
			TArray<FString> Can;
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				const UGameplayAbility* Ability = Spec.GetPrimaryInstance() ? Spec.GetPrimaryInstance() : Spec.Ability.Get();
				if (Ability && Ability->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get())) Can.AddUnique(ObjectId(Ability));
			}
			Can.Sort();
			TArray<TSharedPtr<FJsonValue>> Ids;
			for (const FString& Id : Can) Ids.Add(Str(Id));
			Row.Add(MakeShared<FJsonValueArray>(Ids));
		}
		Role.Frames.Add(MakeShared<FJsonValueArray>(Row));
	}
}

void UCombatRecorder::OnSpawned(AActor* Actor)
{
	// a cue's actor comes from the cue manager's pool, spawned only when the pool has none: the cue itself is recorded
	if (!Actor || Actor->IsA<AGameplayCueNotify_Actor>()) return;
	const FString Name = RoleOf(Actor);
	Event(TEXT("spawn"), Name, {Str(ObjectId(Actor)), Str(RoleOf(Actor->GetOwner())), Str(RoleOf(Actor->GetInstigator())), Vec(Actor->GetActorLocation())});
	// characters (summons) and projectiles are followed frame by frame too; a hitbox is only its spawn
	if (Actor->IsA<ACharacter>() || Actor->IsA<AMyProjectile>())
	{
		Names.Remove(Actor);
		AddRole(Name, Actor);
	}
}

void UCombatRecorder::OnEffect(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle, int32 Role, bool bPeriodic)
{
	TArray<TPair<FString, float>> Magnitudes;
	for (const TPair<FGameplayTag, float>& Pair : Spec.SetByCallerTagMagnitudes) Magnitudes.Emplace(Pair.Key.ToString(), Pair.Value);
	Magnitudes.Sort([](const TPair<FString, float>& A, const TPair<FString, float>& B) { return A.Key < B.Key; });
	TArray<TSharedPtr<FJsonValue>> Mags;
	for (const TPair<FString, float>& Pair : Magnitudes) Mags.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{Str(Pair.Key), Num(Pair.Value)}));
	const FGameplayEffectContextHandle& Context = Spec.GetEffectContext();
	Event(bPeriodic ? TEXT("periodic") : TEXT("effect"), Roles[Role].Name, {
		Str(ObjectId(Spec.Def)), Str(RoleOf(Context.GetInstigator())), Str(RoleOf(Context.GetEffectCauser())),
		Str(ObjectId(Context.GetAbility())), Num(Spec.GetLevel()), Num(Spec.GetDuration()), Num(Spec.GetPeriod()),
		MakeShared<FJsonValueArray>(Mags), Num(Spec.GetStackCount())});
}

void UCombatRecorder::OnEffectRemoved(const FActiveGameplayEffect& Effect, int32 Role)
{
	Event(TEXT("removed"), Roles[Role].Name, {Str(ObjectId(Effect.Spec.Def))});
}

void UCombatRecorder::OnTag(const FGameplayTag Tag, int32 Count, int32 Role)
{
	Event(TEXT("tag"), Roles[Role].Name, {Str(Tag.ToString()), Num(Count)});
}

void UCombatRecorder::OnAttribute(const FOnAttributeChangeData& Data, int32 Role)
{
	Event(TEXT("attribute"), Roles[Role].Name, {Str(Data.Attribute.GetName()), Num(Data.OldValue), Num(Data.NewValue)});
}

void UCombatRecorder::OnActivated(UGameplayAbility* Ability, int32 Role)
{
	Event(TEXT("ability"), Roles[Role].Name, {Str(TEXT("activated")), Str(ObjectId(Ability))});
}

void UCombatRecorder::OnEnded(const FAbilityEndedData& Data, int32 Role)
{
	Event(TEXT("ability"), Roles[Role].Name, {Str(Data.bWasCancelled ? TEXT("cancelled") : TEXT("ended")), Str(ObjectId(Data.AbilityThatEnded))});
}

void UCombatRecorder::OnFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& Tags, int32 Role)
{
	TArray<TSharedPtr<FJsonValue>> Why;
	for (const FGameplayTag& Tag : Tags) Why.Add(Str(Tag.ToString()));
	Event(TEXT("failed"), Roles[Role].Name, {Str(ObjectId(Ability)), MakeShared<FJsonValueArray>(Why)});
}

void UCombatRecorder::OnCue(AActor* Target, FGameplayTag Tag, EGameplayCueEvent::Type Type, const FGameplayCueParameters& Parameters, EGameplayCueExecutionOptions Options)
{
	if (!Target || Target->GetWorld() != World.Get()) return;
	Event(TEXT("cue"), RoleOf(Target), {Str(Tag.ToString()), Num((int32)Type), Num(Parameters.RawMagnitude), Num(Parameters.NormalizedMagnitude),
		Vec(Target->GetActorTransform().InverseTransformPosition(FVector(Parameters.Location)))});
}

void UCombatRecorder::OnProbe(const FAssTestProbeEvent& Probe)
{
	if (Probe.Who && Probe.Who->GetWorld() != World.Get()) return;
	Event(TEXT("probe"), RoleOf(Probe.Who), {Str(Probe.What.ToString()), Str(RoleOf(Probe.Other)), Str(Probe.Detail), Vec(Probe.Value), Num(Probe.Number)});
}
