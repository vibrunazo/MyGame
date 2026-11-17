#include "AssCombatComponent.h"
#include "AssCombatant.h"
#include "HitReactionEffectComponent.h"
#include "MyGameplayAbility.h"
#include "../MyGameplayTags.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "../Dev/TestProbe.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

UAssCombatComponent::UAssCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAssCombatComponent* UAssCombatComponent::Find(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UAssCombatComponent>() : nullptr;
}

ACharacter* UAssCombatComponent::GetCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

UAbilitySystemComponent* UAssCombatComponent::GetAbilitySystem() const
{
	// the owner's, from its construction on: its movement mode is set before it begins play
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}

bool UAssCombatComponent::IsOwnerAlive() const
{
	IAssCombatant* Combatant = Cast<IAssCombatant>(GetOwner());
	return Combatant && Combatant->IsAlive();
}

void UAssCombatComponent::BeginCombat()
{
	ACharacter* Char = GetCharacter();
	if (!Char) return;
	PoseFinalizedHandle = Char->GetMesh()->RegisterOnBoneTransformsFinalizedDelegate(FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &UAssCombatComponent::OnPoseFinalized));
	PreActorTickHandle = FWorldDelegates::OnWorldPreActorTick.AddUObject(this, &UAssCombatComponent::OnFrameStart);
}

void UAssCombatComponent::BindAbilitySystem(UAbilitySystemComponent* InAbilitySystem)
{
	if (!InAbilitySystem) return;
	InAbilitySystem->RegisterGameplayTagEvent(MyGameplayTags::Status_NoPawnBlock, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UAssCombatComponent::PawnBlockTagChanged);
}

void UAssCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACharacter* Char = GetCharacter())
	{
		if (Char->GetMesh()) Char->GetMesh()->UnregisterOnBoneTransformsFinalizedDelegate(PoseFinalizedHandle);
	}
	FWorldDelegates::OnWorldPreActorTick.Remove(PreActorTickHandle);
	Super::EndPlay(EndPlayReason);
}

void UAssCombatComponent::OnPoseFinalized()
{
	HitWindows.RemoveAll([](const TWeakObjectPtr<AHitBox>& Hitbox) { return !Hitbox.IsValid(); });
	for (const TWeakObjectPtr<AHitBox>& Hitbox : TArray<TWeakObjectPtr<AHitBox>>(HitWindows))
	{
		if (Hitbox.IsValid()) Hitbox->AdvanceWindow();
	}
}

void UAssCombatComponent::OnMovementModeChanged()
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	ACharacter* Char = GetCharacter();
	if (!Char || !ASC) return;
	FGameplayTagContainer FlyingTagContainer = FGameplayTagContainer(MyGameplayTags::Activate_FlyingAttack);
	FGameplayTagContainer GroundTagContainer = FGameplayTagContainer(MyGameplayTags::Activate_GroundAttack);
	ASC->SetLooseGameplayTagCount(MyGameplayTags::Status_Airborne, Char->GetMovementComponent()->IsFalling() ? 1 : 0);
	if (!Char->GetMovementComponent()->IsFalling())	// on the ground
	{
		ASC->BlockAbilitiesWithTags(FlyingTagContainer);
		ASC->UnBlockAbilitiesWithTags(GroundTagContainer);
		ASC->CancelAbilities(&FlyingTagContainer);
		Char->GetCapsuleComponent()->SetGenerateOverlapEvents(true);
		Char->GetMesh()->SetGenerateOverlapEvents(false);
	}
	else										// falling
	{
		ASC->BlockAbilitiesWithTags(GroundTagContainer);
		ASC->UnBlockAbilitiesWithTags(FlyingTagContainer);
		ASC->CancelAbilities(&GroundTagContainer);
		Char->GetCapsuleComponent()->SetGenerateOverlapEvents(false);
		Char->GetMesh()->SetGenerateOverlapEvents(true);
	}
}

void UAssCombatComponent::OnHitReaction(EHitReaction Reaction, const FGameplayEffectSpec& Spec)
{
	AActor* SourceActor = Spec.GetEffectContext().GetEffectCauser();
	IAssCombatant* Combatant = Cast<IAssCombatant>(GetOwner());
	switch (Reaction)
	{
	case EHitReaction::HitStun:
		// the effect can't apply while stun immune, so getting here means the stun landed
		if (Combatant) Combatant->PlayHitStunMontage();
		IncrementHitStunCount();
		break;
	case EHitReaction::Knockback:
		ApplyKnockBack(SourceActor, Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_Knockback, false));
		break;
	case EHitReaction::Launch:
		ApplyLaunchBack(SourceActor, FVector(Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_Launch_X, false),
			Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_Launch_Y, false), Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_Launch_Z, false)));
		break;
	case EHitReaction::CameraShake:
		if (Combatant) Combatant->PlayHitCameraShake(Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_CamShake, false));
		break;
	}
}

void UAssCombatComponent::IncrementHitStunCount()
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	HitStunCount++;
	if (MaxStuns > 0 && HitStunCount >= MaxStuns)
	{
		HitStunCount = 0;
		if (!ASC) return;
		const FGameplayEffectSpecHandle Handle = ASC->MakeOutgoingSpec(StunImmuneEffect, 0.f, ASC->MakeEffectContext());
		Handle.Data.Get()->SetSetByCallerMagnitude(MyGameplayTags::Data_StunImmune, StunImmuneCooldown);
		ASC->ApplyGameplayEffectSpecToSelf(*(Handle.Data.Get()));
	}
}

int32 UAssCombatComponent::ResetHitStunCount()
{
	const int32 Was = HitStunCount;
	HitStunCount = 0;
	return Was;
}

bool UAssCombatComponent::HasStunImmune() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	return ASC && ASC->HasMatchingGameplayTag(MyGameplayTags::Status_StunImmune);
}

void UAssCombatComponent::ApplyKnockBack(AActor* SourceActor, float Power)
{
	const AActor* Owner = GetOwner();
	KnockBackVector = (SourceActor ? SourceActor : Owner)->GetActorForwardVector().GetSafeNormal() * Power;
	StartBackslide(KnockBackVector);
}

void UAssCombatComponent::StartBackslide(FVector Dir)
{
	MYGAME_TEST_PROBE(GetOwner(), "backslide", nullptr, FString(), KnockBackVector);
	// ten offsets 0.01 s apart; the engine sets no timer at rate 0, so nine fire (COMBAT.md)
	for (uint8 i = 0; i < 10; i++)
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &UAssCombatComponent::OnBackslide, i * 0.01f, false);
	}
}

void UAssCombatComponent::OnBackslide()
{
	if (AActor* Owner = GetOwner()) Owner->AddActorWorldOffset(KnockBackVector * 0.01f, true);
}

void UAssCombatComponent::ApplyLaunchBack(AActor* SourceActor, FVector Power)
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	const FVector LaunchDir = SourceActor ? SourceActor->GetActorForwardVector() : GetOwner()->GetActorForwardVector();
	const float Angle = LaunchDir.Rotation().Yaw - Power.Rotation().Yaw;
	LastLaunchBack = Power.RotateAngleAxis(Angle, FVector(0.f, 0.f, 1.f));
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UAssCombatComponent::OnDelayedLaunch, 0.05f, false);
	if (!ASC) return;
	const FGameplayEffectSpecHandle Handle = ASC->MakeOutgoingSpec(NoControlEffect, 0.f, ASC->MakeEffectContext());
	Handle.Data.Get()->SetSetByCallerMagnitude(MyGameplayTags::Data_NoControl, 0.5f);
	ASC->ApplyGameplayEffectSpecToSelf(*(Handle.Data.Get()));
}

void UAssCombatComponent::OnDelayedLaunch()
{
	if (ACharacter* Char = GetCharacter()) Char->LaunchCharacter(LastLaunchBack, true, true);
}

bool UAssCombatComponent::HasControl() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	if (ASC && (ASC->HasMatchingGameplayTag(MyGameplayTags::Status_HitStun) || ASC->HasMatchingGameplayTag(MyGameplayTags::Status_NoControl)))
	{
		return false;
	}
	return bControlEnabled;
}

bool UAssCombatComponent::HasMoveControl() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	return !(ASC && (ASC->HasMatchingGameplayTag(MyGameplayTags::Status_HitStun)
		|| ASC->HasMatchingGameplayTag(MyGameplayTags::Status_NoControl)
		|| ASC->HasMatchingGameplayTag(MyGameplayTags::Status_NoMove)));
}

void UAssCombatComponent::PawnBlockTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	RefreshPawnCollision();
}

void UAssCombatComponent::RefreshPawnCollision()
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	ACharacter* Char = GetCharacter();
	if (!Char) return;
	UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
	const bool bIgnorePawns = !IsOwnerAlive()
		|| (Char->GetCharacterMovement() && Char->GetCharacterMovement()->IsFalling())
		|| (ASC && ASC->HasMatchingGameplayTag(MyGameplayTags::Status_NoPawnBlock));
	if (bIgnorePawns)
	{
		GetWorld()->GetTimerManager().ClearTimer(PawnBlockRetryTimer);
		PawnBlockWaitStart = -1.f;
		Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		return;
	}
	// Blocking again while inside another character makes movement shove the capsules apart, which can push
	// one into level geometry. Wait briefly for them to separate; if they don't (an enemy standing in us),
	// step out sideways ourselves, swept so we never go through walls
	if (const ACharacter* Other = FindOverlappingCharacter())
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (PawnBlockWaitStart < 0.f) PawnBlockWaitStart = Now;
		if (Now - PawnBlockWaitStart < 0.3f)
		{
			GetWorld()->GetTimerManager().SetTimer(PawnBlockRetryTimer, this, &UAssCombatComponent::RefreshPawnCollision, 0.05f, false);
			return;
		}
		StepOutOf(Other);
	}
	PawnBlockWaitStart = -1.f;
	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
}

const ACharacter* UAssCombatComponent::FindOverlappingCharacter() const
{
	const ACharacter* Char = GetCharacter();
	const UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PawnOverlap), false, Char);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Capsule->GetComponentLocation(), Capsule->GetComponentQuat(),
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), Capsule->GetCollisionShape(), Params);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const ACharacter* Other = Cast<ACharacter>(Overlap.GetActor());
		// only other characters' capsules count; their meshes overlap pawns by design
		if (Other && Overlap.GetComponent() == Other->GetCapsuleComponent()) return Other;
	}
	return nullptr;
}

void UAssCombatComponent::StepOutOf(const ACharacter* Other)
{
	ACharacter* Char = GetCharacter();
	FVector Away = Char->GetActorLocation() - Other->GetActorLocation();
	Away.Z = 0.f;
	if (!Away.Normalize()) Away = -Char->GetActorForwardVector().GetSafeNormal2D();
	const float Needed = Char->GetCapsuleComponent()->GetScaledCapsuleRadius() + Other->GetCapsuleComponent()->GetScaledCapsuleRadius()
		- FVector::Dist2D(Char->GetActorLocation(), Other->GetActorLocation()) + 2.f;
	if (Needed > 0.f) Char->SetActorLocation(Char->GetActorLocation() + Away * Needed, true);
}

void UAssCombatComponent::OnHitPause(float Duration)
{
	AActor* Owner = GetOwner();
	if (!GetWorld() || !Owner) return;
	const double Now = GetWorld()->GetTimeSeconds();
	// the pause starts when the hit connected, which may be earlier in this frame
	const double Start = LastHitConnectFrame == GFrameCounter ? FMath::Min(LastHitConnectTime, Now) : Now;
	const double End = Start + Duration;
	// this frame ran on at its own speed past the hit: owe back what the pause would have taken
	HitPauseDebt += FMath::Max(0.0, (Owner->CustomTimeDilation - HitPauseRate) * (FMath::Min(Now, End) - Start));
	if (!bHitPaused || Now >= HitPauseEnd) HitPauseStart = Start;
	HitPauseEnd = FMath::Max(HitPauseEnd, End);
	Owner->CustomTimeDilation = HitPauseRate;
	if (!bHitPaused) SetHitPauseFX(true);
	bHitPaused = true;
}

void UAssCombatComponent::OnFrameStart(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	ACharacter* Char = GetCharacter();
	if (!Char || World != GetWorld() || DeltaSeconds <= 0.f) return;
	// world time has already moved on to this frame's end; nothing has moved yet
	FrameStartPose.Capture(Char, World->GetTimeSeconds() - DeltaSeconds);
	if (!bHitPaused)
	{
		// the frame the pause ended in ran partly paused; the ones after run at full speed
		Char->CustomTimeDilation = 1.f;
		ApplyRootSpeedCurve(DeltaSeconds);
		return;
	}
	// this frame covers world time T0..T1; it should run at full speed except where it overlaps the pause
	const double T1 = World->GetTimeSeconds();
	const double T0 = T1 - DeltaSeconds;
	const double Paused = FMath::Max(0.0, FMath::Min(T1, HitPauseEnd) - FMath::Max(T0, HitPauseStart));
	const double Ideal = DeltaSeconds - (1.0 - HitPauseRate) * Paused;
	const double Given = FMath::Clamp(Ideal - HitPauseDebt, HitPauseRate * DeltaSeconds, (double)DeltaSeconds);
	HitPauseDebt += Given - Ideal;
	Char->CustomTimeDilation = Given / DeltaSeconds;
	if (T1 >= HitPauseEnd && HitPauseDebt <= KINDA_SMALL_NUMBER)
	{
		HitPauseDebt = 0.0;
		bHitPaused = false;
		SetHitPauseFX(false);
	}
	ApplyRootSpeedCurve(DeltaSeconds);
}

void UAssCombatComponent::ApplyRootSpeedCurve(float DeltaSeconds)
{
	static const FName CurveName(TEXT("RootSpeedX"));
	static const FName SourceName(TEXT("RootSpeedCurve"));
	ACharacter* Char = GetCharacter();
	const float MoveTime = DeltaSeconds * Char->CustomTimeDilation;
	const float PreviousMoveTime = LastMoveTime;
	LastMoveTime = MoveTime;
	UCharacterMovementComponent* Move = Char->GetCharacterMovement();
	const UAnimInstance* Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
	const UAnimMontage* Montage = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
	const UMyGameplayAbility* Ability = Cast<UMyGameplayAbility>(UMyBlueprintFunctionLibrary::GetAnimatingAbilityFromActor(Char));
	if (!Move || !Montage || !Ability || !Montage->HasCurveData(CurveName)) return;
	// the montage time this frame plays, plus any its curve hasn't moved the character for yet: a montage that started
	// during the last frame already played part of it
	const float Rate = Anim->Montage_GetPlayRate(Montage) * Montage->RateScale;
	const float Now = Anim->Montage_GetPosition(Montage);
	// a new montage (or the same one restarted): it began playing during the last frame
	if (RootSpeedMontage != Montage || RootSpeedCovered - Now > PreviousMoveTime * Rate + KINDA_SMALL_NUMBER)
	{
		RootSpeedMontage = Montage;
		RootSpeedCovered = FMath::Max(0.f, Now - PreviousMoveTime * Rate);
	}
	// (if the last frame played less than it planned, as when a hit paused it, this frame's span is that much shorter)
	const float From = RootSpeedCovered;
	const float Span = Now + MoveTime * Rate - From;
	RootSpeedCovered = From + Span;
	if (MoveTime <= 0.f || Span <= 0.f) return;
	// The curves were tuned at 60 fps, where the velocity they set was braked for one frame before it moved the
	// character: each 1/60 s of curve moves it at that braked speed, so the moves keep their tuned distances
	const float Step = 1.f / 60.f;
	const float Friction = (Move->bUseSeparateBrakingFriction ? Move->BrakingFriction : Move->GroundFriction) * FMath::Max(0.f, Move->BrakingFrictionFactor);
	const float Braking = Move->GetMaxBrakingDeceleration();
	const int32 Samples = FMath::Clamp(FMath::CeilToInt(Span * 240.f), 1, 64);
	float Sum = 0.f;
	for (int32 Sample = 0; Sample < Samples; ++Sample)
	{
		const float Set = 100.f * Ability->RootMotionScale.X * Montage->EvaluateCurveData(CurveName, FAnimExtractContext((double)(From + Span * (Sample + 0.5f) / Samples)));
		Sum += Set > 0.f ? FMath::Max(0.f, Set - (Friction * Set + Braking) * Step) : 0.f;
	}
	// the distance over this frame's span, covered in this frame's movement time
	const float Speed = Sum / Samples * Span / (MoveTime * Rate);
	// past the curve's end the last velocity is kept and brakes as usual
	if (Speed <= 0.f) return;
	TSharedPtr<FRootMotionSource_ConstantForce> Source = MakeShared<FRootMotionSource_ConstantForce>();
	Source->InstanceName = SourceName;
	Source->AccumulateMode = ERootMotionAccumulateMode::Override;
	Source->Priority = 5;
	Source->Force = Char->GetActorForwardVector() * Speed;
	Source->Duration = MoveTime;
	Source->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	Move->RemoveRootMotionSource(SourceName);
	Move->ApplyRootMotionSource(Source);
}

void UAssCombatComponent::SetHitPauseFX(bool bPaused)
{
	ACharacter* Char = GetCharacter();
	if (!Char || !Char->GetMesh()) return;
	TArray<USceneComponent*> MeshChildren;
	Char->GetMesh()->GetChildrenComponents(false, MeshChildren);
	for (USceneComponent* Component : MeshChildren)
	{
		if (UNiagaraComponent* FXSystemComponent = Cast<UNiagaraComponent>(Component))
		{
			FXSystemComponent->SetPaused(bPaused);
		}
	}
}
