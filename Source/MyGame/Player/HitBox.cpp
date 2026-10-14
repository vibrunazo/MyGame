// Fill out your copyright notice in the Description page of Project Settings.


#include "HitBox.h"
#include "../MyGameplayTags.h"
#include "HitboxSettings.h"
#include "../Abilities/MyGameplayAbility.h"
#include "../Abilities/IGetHit.h"

#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "MyCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "../Abilities/ANS_RotateChar.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AHitBox::AHitBox()
{
	PrimaryActorTick.bCanEverTick = true;

	MyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = MyRoot;
	//OnActorBeginOverlap.AddDynamic(this, &AHitBox::OnHitboxBeginOverlap);
	// bGenerateOverlapEvents = true;
	// overlapeve
	// MySphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Collision"));
	// MySphere->SetupAttachment(RootComponent);
	// MySphere->SetSphereRadius
}

// Called when the game starts or when spawned
void AHitBox::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwner())	{
		// UE_LOG(LogTemp, Warning, TEXT("I am hitbox and %s is my daddy"), *GetOwner()->GetName());
		GetOwner()->OnDestroyed.AddDynamic(this, &AHitBox::OnOwnerDestroyed);
	}
	// else {UE_LOG(LogTemp, Warning, TEXT("I am hitbox and I has no daddy"));}
}

void AHitBox::OnOwnerDestroyed(AActor* DestroyedActor)
{
	Destroy();
}


// Instigator is the PLAYER PAWN
// Owner is the Actor who spawned the Hitbox (either player or a projectile)

void AHitBox::HitboxTouched(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!GetWorld() || !OverlappedComp) return;
	TryHit(Other, OverlappedComp->GetComponentLocation(), GetWorld()->GetTimeSeconds());
}

void AHitBox::TryHit(AActor* Other, const FVector& From, double Time)
{
	if (!GetWorld() || !Other) return;
	IGetHit* Target = Cast<IGetHit>(Other);
	IGetHit* Source = Cast<IGetHit>(GetInstigator());
	if (!Target || !Source) return;
	if (!ensure(GetInstigator() != nullptr)) return;
	if (HitboxChannel == EHitboxChannel::Projectile && Target->IsProjectileImmune())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot hit target because it's immune to projectiles"));
		return;
	}
	FEnemyHitState CurEnemyState = ActorsHit.FindOrAdd(Other);
	const bool bHitMaxTimes = CurEnemyState.NumHits >= NumHits;
	const bool bCooledDown = CurEnemyState.NumHits == 0 || Time - CurEnemyState.LastHitTime > HitCooldown;
	if (GetInstigator() == Other || !Target->IsAlive() || bHitMaxTimes || !bCooledDown || Source->GetTeam() == Target->GetTeam()) return;
	FHitResult OutHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	Params.AddIgnoredActor(GetInstigator());
	if (!GetWorld()->LineTraceSingleByChannel(OutHit, From, Other->GetActorLocation(), ECollisionChannel::ECC_Visibility, Params, FCollisionResponseParams()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Hittrace failed"));
		return;
	}
	CurEnemyState.NumHits++;
	CurEnemyState.LastHitTime = Time;
	ActorsHit.Emplace(Other, CurEnemyState);
	// the attacker's hit pause starts at this contact, not at the end of the frame
	if (AMyCharacter* Attacker = Cast<AMyCharacter>(GetInstigator())) Attacker->NoteHitConnect(Time);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetInstigator(), MyGameplayTags::Notify_Hit_Connect, FGameplayEventData());
	ApplyAllEffects(Target);
	// sound and sparks: a stun-immune target blocks
	if (UAbilitySystemComponent* GAS = Target->GetAbilitySystemComponent())
	{
		FGameplayCueParameters Cue;
		Cue.Location = OutHit.ImpactPoint;
		Cue.Normal = OutHit.ImpactNormal;
		Cue.SourceObject = this;
		Cue.EffectCauser = GetOwner();
		Cue.Instigator = GetInstigator();
		Cue.RawMagnitude = GAS->HasMatchingGameplayTag(MyGameplayTags::Status_StunImmune) ? 1.f : 0.f;
		GAS->ExecuteGameplayCue(MyGameplayTags::GameplayCue_Hit_Impact, Cue);
	}
}

void FHitboxPose::Capture(const ACharacter* Char, double AtTime)
{
	const USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : nullptr;
	if (!Mesh) return;
	const UAnimInstance* Anim = Mesh->GetAnimInstance();
	const UAnimMontage* Active = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
	Montage = Active;
	MontagePosition = Active ? Anim->Montage_GetPosition(Active) : 0.f;
	Time = AtTime;
	MeshToWorld = Mesh->GetComponentTransform();
	ActorToWorld = Char->GetActorTransform();
}

void AHitBox::BeginWindow(AMyCharacter* Char, const UAnimMontage* Montage, float Start, float End, const TArray<FHitboxSettings>& Settings, uint8 InNumHits, float InHitCooldown)
{
	NumHits = InNumHits;
	HitCooldown = InHitCooldown;
	WindowChar = Char;
	WindowMontage = Montage;
	WindowEnd = FMath::Max(Start, End);
	NextSample = Start;
	bWindowDone = false;
	const USkeletalMeshComponent* Mesh = Char->GetMesh();
	const USkeleton* Skeleton = Mesh->GetSkeletalMeshAsset() ? Mesh->GetSkeletalMeshAsset()->GetSkeleton() : nullptr;
	for (const FHitboxSettings& Setting : Settings)
	{
		TArray<FName> Bones = Setting.BoneNames;
		if (Bones.Num() == 0) Bones.Add(NAME_None);
		for (const FName& Name : Bones)
		{
			FWindowShape& Shape = WindowShapes.AddDefaulted_GetRef();
			Shape.Settings = Setting;
			if (Name == NAME_None) continue;
			// a bone, or a socket on one
			Shape.Bone = Mesh->GetBoneIndex(Name);
			if (Shape.Bone == INDEX_NONE)
			{
				if (const USkeletalMeshSocket* Socket = Mesh->GetSocketByName(Name))
				{
					Shape.Bone = Mesh->GetBoneIndex(Socket->BoneName);
					Shape.SocketOffset = Socket->GetSocketLocalTransform();
				}
			}
			if (Skeleton && Shape.Bone != INDEX_NONE)
			{
				const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
				for (int32 Index = RefSkeleton.FindBoneIndex(Mesh->GetBoneName(Shape.Bone)); Index != INDEX_NONE; Index = RefSkeleton.GetParentIndex(Index))
				{
					Shape.SkeletonChain.Add(Index);
				}
			}
		}
	}
	Char->TrackHitWindow(this);
	// the window opened during this animation update: test it up to now
	AdvanceWindow();
}

void AHitBox::AdvanceWindow()
{
	const AMyCharacter* Char = WindowChar.Get();
	const UAnimInstance* Anim = Char ? Char->GetMesh()->GetAnimInstance() : nullptr;
	const UAnimMontage* Montage = WindowMontage.Get();
	// a montage that stopped tests no more of its window
	if (bWindowDone || !Anim || !Montage || !Anim->Montage_IsActive(Montage)) return;
	SampleWindowUntil(FMath::Min(Anim->Montage_GetPosition(Montage), WindowEnd));
}

void AHitBox::FinishWindow()
{
	SampleWindowUntil(WindowEnd);
}

void AHitBox::SampleWindowUntil(float Position)
{
	const AMyCharacter* Char = WindowChar.Get();
	const USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : nullptr;
	const UAnimInstance* Anim = Mesh ? Mesh->GetAnimInstance() : nullptr;
	const UAnimMontage* Montage = WindowMontage.Get();
	if (bWindowDone || !Anim || !Montage || !GetWorld()) return;
	// the frame started with the montage at P0 and is at P1 now: a sample's montage time says how far into the frame it is
	const FHitboxPose& Start = Char->GetFrameStartPose();
	const float P1 = Anim->Montage_IsActive(Montage) ? Anim->Montage_GetPosition(Montage) : Position;
	const bool bInterpolate = Start.Montage == Montage && Start.MontagePosition <= P1;
	const float P0 = bInterpolate ? Start.MontagePosition : P1;
	const double T1 = GetWorld()->GetTimeSeconds();
	const double T0 = bInterpolate ? Start.Time : T1;
	while (!bWindowDone && NextSample <= Position + KINDA_SMALL_NUMBER)
	{
		const float Alpha = P1 > P0 ? FMath::Clamp((NextSample - P0) / (P1 - P0), 0.f, 1.f) : 1.f;
		SampleWindowAt(FMath::Min(NextSample, WindowEnd), Alpha, bInterpolate, FMath::Lerp(T0, T1, (double)Alpha));
		bWindowDone = NextSample >= WindowEnd - KINDA_SMALL_NUMBER;
		// the last step is cut short so the window's end is always tested
		NextSample = FMath::Min(NextSample + WindowStep, WindowEnd);
	}
}

bool AHitBox::AnimBoneTransform(const FWindowShape& Shape, float Position, FTransform& OutComponentSpace) const
{
	const UAnimMontage* Montage = WindowMontage.Get();
	if (!Montage || Shape.SkeletonChain.Num() == 0) return false;
	for (const FSlotAnimationTrack& Slot : Montage->SlotAnimTracks)
	{
		const FAnimSegment* Segment = Slot.AnimTrack.GetSegmentAtTime(Position);
		float AnimPosition = 0.f;
		const UAnimSequence* Sequence = Segment ? Cast<UAnimSequence>(Segment->GetAnimationData(Position, AnimPosition)) : nullptr;
		if (!Sequence) continue;
		// the bone's local transforms composed up to the root; with root motion the root stays where it's locked
		const FAnimExtractContext Context(AnimPosition);
		OutComponentSpace = FTransform::Identity;
		for (const int32 Index : Shape.SkeletonChain)
		{
			FTransform Local;
			if (Index == 0 && Sequence->bEnableRootMotion)
			{
				switch (Sequence->RootMotionRootLock)
				{
				case ERootMotionRootLock::AnimFirstFrame:
					Sequence->GetBoneTransform(Local, FSkeletonPoseBoneIndex(0), FAnimExtractContext(0.0), false);
					break;
				case ERootMotionRootLock::Zero:
					Local = FTransform::Identity;
					break;
				default:
					Local = Sequence->GetSkeleton()->GetReferenceSkeleton().GetRefBonePose()[0];
				}
			}
			else
			{
				Sequence->GetBoneTransform(Local, FSkeletonPoseBoneIndex(Index), Context, false);
			}
			OutComponentSpace = OutComponentSpace * Local;
		}
		return true;
	}
	return false;
}

void AHitBox::SampleWindowAt(float At, float Alpha, bool bInterpolate, double Time)
{
	const AMyCharacter* Char = WindowChar.Get();
	const USkeletalMeshComponent* Mesh = Char->GetMesh();
	// the mesh and actor where they were at this sample: between where the frame started and where it ended
	const FHitboxPose& Start = Char->GetFrameStartPose();
	FTransform MeshToWorld = Mesh->GetComponentTransform();
	FTransform ActorToWorld = Char->GetActorTransform();
	if (bInterpolate)
	{
		MeshToWorld.Blend(Start.MeshToWorld, Mesh->GetComponentTransform(), Alpha);
		ActorToWorld.Blend(Start.ActorToWorld, Char->GetActorTransform(), Alpha);
	}
	// a spinning mesh can turn more than half a turn in a long frame: take its rotation from the spin itself
	FQuat SpinRotation;
	if (UANS_RotateChar::MeshRotationAt(Mesh, WindowMontage.Get(), At, SpinRotation))
	{
		FTransform MeshToActor = Mesh->GetRelativeTransform();
		MeshToActor.SetRotation(SpinRotation);
		MeshToWorld = MeshToActor * ActorToWorld;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HitWindow), false, GetOwner());
	Params.AddIgnoredActor(GetInstigator());
	for (FWindowShape& Shape : WindowShapes)
	{
		// the shape's parent at this sample: a bone posed by the montage's animation at its exact time (the
		// mesh's own pose is only as fresh as the last frame), or the actor
		FTransform Parent = ActorToWorld;
		if (Shape.Bone != INDEX_NONE)
		{
			FTransform BoneInComponent;
			Parent = AnimBoneTransform(Shape, At, BoneInComponent) ? BoneInComponent * MeshToWorld : Mesh->GetBoneTransform(Shape.Bone);
		}
		const FTransform World = Shape.Settings.HitboxTransform * Shape.SocketOffset * Parent;
		const FVector Scale = World.GetScale3D().GetAbs();
		const float Radius = Shape.Settings.SphereRadius * Scale.GetMin();
		const FCollisionShape Collision = Shape.Settings.bIsSphere ? FCollisionShape::MakeSphere(Radius) : FCollisionShape::MakeBox(Shape.Settings.BoxExtent * Scale);
		// candidates around the shape; a character is tested where it was at this sample too, not where its frame ended
		TArray<FOverlapResult> Overlaps;
		const FCollisionShape Around = Shape.Settings.bIsSphere ? FCollisionShape::MakeSphere(Radius + CandidateMargin) : FCollisionShape::MakeBox(Shape.Settings.BoxExtent * Scale + FVector(CandidateMargin));
		GetWorld()->OverlapMultiByObjectType(Overlaps, World.GetLocation(), World.GetRotation(), FCollisionObjectQueryParams(ECC_Pawn), Around, Params);
		TSet<TWeakObjectPtr<AActor>> Touching;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Other = Overlap.GetActor();
			if (!Other || Touching.Contains(Other)) continue;
			bool bTouches;
			const AMyCharacter* Target = Cast<AMyCharacter>(Other);
			if (Shape.Settings.bIsSphere && Target && Target->GetCapsuleComponent())
			{
				const FHitboxPose& TargetStart = Target->GetFrameStartPose();
				const double Now = GetWorld()->GetTimeSeconds();
				const double Beta = Now > TargetStart.Time ? FMath::Clamp((Time - TargetStart.Time) / (Now - TargetStart.Time), 0.0, 1.0) : 1.0;
				const FVector Center = FMath::Lerp(TargetStart.ActorToWorld.GetLocation(), Target->GetActorLocation(), Beta);
				const UCapsuleComponent* Capsule = Target->GetCapsuleComponent();
				const FVector Axis = Capsule->GetUpVector() * (Capsule->GetScaledCapsuleHalfHeight() - Capsule->GetScaledCapsuleRadius());
				bTouches = FMath::PointDistToSegment(World.GetLocation(), Center - Axis, Center + Axis) <= Radius + Capsule->GetScaledCapsuleRadius();
			}
			else
			{
				bTouches = Overlap.Component.IsValid() && Overlap.Component->OverlapComponent(World.GetLocation(), World.GetRotation(), Collision);
			}
			if (!bTouches) continue;
			Touching.Add(Other);
			if (!Shape.Touching.Contains(Other)) TryHit(Other, World.GetLocation(), Time);
		}
		Shape.Touching = MoveTemp(Touching);
	}
}

// Called every frame
void AHitBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

//void AHitBox::AddComponentsToBones(TArray<FName> Bones)
//{
//	for (auto &&Bone : Bones)
//	{
//		USphereComponent* NewSphere = AddHitSphere();
//		// NewSphere->bGenerateOverlapEvents = true;
//		if (!ensure(GetInstigator() != nullptr)) return;
//		// if (!ensure(GetOwner() != nullptr)) return;
//		USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GetInstigator()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
//		NewSphere->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
//	}
//}

void AHitBox::SetOwningAbility(UMyGameplayAbility* NewOwner)
{
	OwningAbility = NewOwner;
}

void AHitBox::SetupHitboxes(const TArray<FHitboxSettings>& Settings, uint8 InNumHits, float InHitCooldown)
{
	NumHits = InNumHits;
	HitCooldown = InHitCooldown;
	for (const FHitboxSettings& Setting : Settings)
	{
		AddComponentsFromSettings(Setting);
	}
}

void AHitBox::AddComponentsFromSettings(FHitboxSettings Settings)
{
	if (!ensure(GetInstigator() != nullptr)) return;
	TArray<FName> Bones = Settings.BoneNames;
	for (auto&& Bone : Bones)
	{
		AddOneComponentFromBone(Settings, Bone);
	}
	if (Bones.Num() == 0)
	{
		AddOneComponentFromBone(Settings, NAME_None);
	}
}

void AHitBox::AddOneComponentFromBone(FHitboxSettings Settings, FName Bone)
{
	UPrimitiveComponent* NewHitComponent;
	if (Settings.bIsSphere)	NewHitComponent = AddHitSphere(Settings.SphereRadius);
	else NewHitComponent = AddHitBoxComponent(Settings.BoxExtent);
	HitComponents.Add(NewHitComponent);
	if (Bone != NAME_None)
	{
		USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GetInstigator()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		NewHitComponent->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
	}
	NewHitComponent->SetRelativeTransform(Settings.HitboxTransform);
	//NewHitComponent->SetWorldScale3D(FVector(1.f, 1.f, 0.1f));
	NewHitComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &AHitBox::HitboxTouched);
	TArray<AActor*> OutOverlappingActors;
	NewHitComponent->GetOverlappingActors(OutOverlappingActors);
	if (OutOverlappingActors.Num() > 0)
	{
		for (auto&& TouchedActor : OutOverlappingActors)
		{
			TryHit(TouchedActor, NewHitComponent->GetComponentLocation(), GetWorld()->GetTimeSeconds());
		}
	}
}

USphereComponent* AHitBox::AddHitSphere(float SphereRadius)
{
	USphereComponent* NewComp = NewObject<USphereComponent>(this);
	NewComp->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	NewComp->RegisterComponent();
	NewComp->SetSphereRadius(SphereRadius);
	NewComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	NewComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	return NewComp;
}

UBoxComponent* AHitBox::AddHitBoxComponent(FVector BoxExtent)
{
	UBoxComponent* NewComp = NewObject<UBoxComponent>(this);
	NewComp->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	NewComp->RegisterComponent();
	NewComp->SetBoxExtent(BoxExtent);
	NewComp->OnComponentBeginOverlap.AddUniqueDynamic(this, &AHitBox::HitboxTouched);
	return NewComp;
}

void AHitBox::ApplyAllEffects(class IGetHit* Target)
{
	if (!Target) return;
	for (auto &&Effect : EffectsToApply)
	{
		ApplyOneEffect(Effect, Target);
	}
	
}

void AHitBox::ApplyOneEffect(FGameplayEffectSpecHandle Effect, class IGetHit* Target)
{
	if (!Target || !IsValid(Cast<UObject>(Target))) return;
	// reactions push the target along the attacker's facing: for a projectile that's the projectile, not its caster
	if (Effect.Data.IsValid())
	{
		FGameplayEffectContextHandle Context = Effect.Data->GetContext();
		Context.AddInstigator(Context.GetInstigator(), GetOwner());
	}
	Target->OnGetHitByEffect(Effect);
}