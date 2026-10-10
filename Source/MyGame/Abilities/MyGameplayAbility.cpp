// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameplayAbility.h"
#include "../MyGameplayTags.h"
#include "IGetHit.h"
#include "../Player/MyCharacter.h"
#include "../Player/HitBox.h"
#include "../Player/HitboxSettings.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
// #include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
// #include "../MyBlueprintFunctionLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UMyGameplayAbility::UMyGameplayAbility()
{
    // Soft path: loading Blueprints from a native constructor deadlocks UE5's loader
    HitBoxClass = TSoftClassPtr<AHitBox>(FSoftObjectPath(TEXT("/Game/Blueprints/Chars/BP_HitBox.BP_HitBox_C")));
}

bool UMyGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const
{
    //UE_LOG(LogTemp, Warning, TEXT("Trying Ability: %s on %s"), *GetName(), *GetAvatarActorFromActorInfo()->GetName());
    FGameplayTag AttackTag = MyGameplayTags::State_Attacking;
    // This tag should be used when the ability is in a State where other abilties can cancel its animation to combo into some other ability
    FGameplayTag CanCancelState = MyGameplayTags::Combo_CanCancel;
    // If I'm in the middle of an attack
    if(ActorInfo->AbilitySystemComponent.Get()->HasMatchingGameplayTag(AttackTag))
    {
        // check if I can cancel an attack of the current type
        if (ActorInfo->AbilitySystemComponent.Get()->HasAnyMatchingGameplayTags(TagsIcanCancel))
        {
            // check if I need a combo hit to cancel and if I have hit, by checking if the CanCancelState Tag was applied by any ability
            if (bNeedsHitToCancel && ActorInfo->AbilitySystemComponent.Get()->HasMatchingGameplayTag(CanCancelState))
            {
                // it can be cancelled so call Super to do regular checks if I can cast this ability 
                return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
            }
            if (!bNeedsHitToCancel)
            {
                // if I can cancel this ability and don't require a hit, so cancel it anyway even if I didn't hit
                return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
            }
        }
        // or else, I cannot activate the ability since I'm in the middle of an attack and I can't cancel it
        return false;
    }
    // if not in an attack just perform regular checks to see if the ability can be activated and activate it
    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UMyGameplayAbility::PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData)
{
    // Decide before Super cancels the attack we're cancelling (its state.attacking / combo.cancancel go with it):
    // an ability started from a hit-confirmed cancel plays its montage from the ComboStart section
    const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    bStartedFromComboCancel = bNeedsHitToCancel && ASC
        && ASC->HasMatchingGameplayTag(MyGameplayTags::State_Attacking)
        && ASC->HasMatchingGameplayTag(MyGameplayTags::Combo_CanCancel)
        && ASC->HasAnyMatchingGameplayTags(TagsIcanCancel);
    Super::PreActivate(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData);
}

void UMyGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo * ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData * TriggerEventData)
{
    if (MontagesToPlay.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No Montages in Ability"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
        return;
    }
    if (!IsValid(GetAvatarActorFromActorInfo())) return;
    CommitAbility(Handle, ActorInfo, ActivationInfo);
    AMyCharacter* MyChar = Cast<AMyCharacter>(GetAvatarActorFromActorInfo()); 
    if (bStartsCombat)
    {
        if (MyChar)
        {
            MyChar->SetIsInCombat(true);
        }
    }
    if (MyChar) MyChar->OnCastDelegate.Broadcast(this);
    //UE_LOG(LogTemp, Warning, TEXT("Activating Ability: %s on %s"), *GetName(), *GetAvatarActorFromActorInfo()->GetName());
    // ResetHitBoxes();
    UpdateCombo();
    bHasHitConnected = false;
    bCanComboState = false;
    bHasHitStarted = false;
    CurHits = 0;
    APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
    if (bUpdateRotationFromController && AvatarPawn)
    {
        FRotator NewRot = AvatarPawn->GetActorRotation();
        NewRot.Yaw = AvatarPawn->GetControlRotation().Yaw;
        AvatarPawn->SetActorRotation(NewRot);
    }
    if (bResetTarget) ResetTarget();
    if (bAqcuireNewTargetFromDetectionBox) AcquireNewTarget();
    if (bLockRotationToTarget) LockToTarget();
    //if (bAlwaysLockRot) LockRot();
    ApplySelfEffects();

    FName MontageSection = NAME_None;
    FGameplayTag CanCancelState = MyGameplayTags::Combo_CanCancel;
    if (bStartedFromComboCancel && MontagesToPlay[CurrentComboCount]->IsValidSectionName(TEXT("ComboStart")))
    {
        MontageSection = "ComboStart";
    }
    GetActorInfo().AbilitySystemComponent.Get()->RemoveLooseGameplayTag(CanCancelState);
    UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontagesToPlay[CurrentComboCount], GetAttackSpeed(), MontageSection, false, 1.0f);
    Task->OnCompleted.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->OnInterrupted.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->OnCancelled.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->OnBlendOut.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->ReadyForActivation();

    FGameplayTag HitStartTag = MyGameplayTags::Notify_Hit_Start;;
    FGameplayTag HitEndTag = MyGameplayTags::Notify_Hit_End;
    FGameplayTag HitConnectTag = MyGameplayTags::Notify_Hit_Connect;

    UAbilityTask_WaitGameplayEvent* HitStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitStartTag);
    HitStartTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitStart);
    HitStartTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* HitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitEndTag);
    HitEndTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitEnd);
    HitEndTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* HitConnectTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitConnectTag);
    HitConnectTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitConnect);
    HitConnectTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* DeactivateTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TagThatDeactivateMe);
    DeactivateTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnDeactivateEvent);
    DeactivateTask->ReadyForActivation();
    
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMyGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    RemoveSelfEffects();
    // try to unlock rotation
    /*if (!bLockRotationToTarget && !bAlwaysLockRot) return;
    AMyCharacter* MyChar = Cast<AMyCharacter>(GetAvatarActorFromActorInfo());
    if (!MyChar) return;
    UCharacterMovementComponent* Move = Cast<UCharacterMovementComponent>(MyChar->GetMovementComponent());
    if (Move) Move->RotationRate = InitialRotRate;
    UE_LOG(LogTemp, Warning, TEXT("EndAbility, set RotationRate to %s"), *InitialRotRate.ToString());*/

}

void UMyGameplayAbility::OnMontageComplete()
{
    // bound to Completed, Interrupted, Cancelled and BlendOut: only the first one ends the ability
    if (!IsActive()) return;
    bHasHitConnected = false;
    bHasHitStarted = false;
    if (!IsValid(GetAvatarActorFromActorInfo())) return;
    ResetHitBoxes();
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
    FGameplayTag CanCancelState = MyGameplayTags::Combo_CanCancel;
    GetActorInfo().AbilitySystemComponent.Get()->RemoveLooseGameplayTag(CanCancelState);
}

void UMyGameplayAbility::OnHitStart(const FGameplayEventData Payload)
{
    if (!IsValid(GetAvatarActorFromActorInfo())) return;
    ResetHitBoxes();
    bHasHitStarted = true;
    // UE_LOG(LogTemp, Warning, TEXT("Hit started"));
    // TODO might crash if I'm dead?
    FVector Loc = GetAvatarActorFromActorInfo()->GetActorLocation();
    FActorSpawnParameters params;
    params.bNoFail = true;
    params.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
    params.Owner = GetAvatarActorFromActorInfo();
    AHitBox* NewHB = GetWorld()->SpawnActor<AHitBox>(GetHitBoxClass(), Loc, FRotator::ZeroRotator, params);
    NewHB->AttachToActor(GetAvatarActorFromActorInfo(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    NewHB->HitSound = HitSound;
    NewHB->BlockSound = BlockSound;
    NewHB->HitParticles = HitParticles;
    NewHB->BlockParticles = BlockParticles;
    NewHB->EffectsToApply = MakeSpecHandles();
    // const UHitboxSettings* Settings = Cast<UHitboxSettings>(&Payload.OptionalObject);
    const UObject* OO = Payload.OptionalObject;
    if (OO) {
        const UHitboxesContainer* Settings = Cast<UHitboxesContainer>(Payload.OptionalObject);
        if (!ensure(Settings != nullptr)) return;
        NewHB->SetOwningAbility(this);
        NewHB->AddComponentsFromContainer(Settings);
    }

    HitBoxRef = NewHB;
}

void UMyGameplayAbility::OnHitEnd(const FGameplayEventData Payload)
{
     //UE_LOG(LogTemp, Warning, TEXT("Hit ended"));
    if (!IsValid(GetAvatarActorFromActorInfo())) return;
    if (!bHasHitConnected && bHasHitStarted && !bCanCancelAfterHitboxEnds) ResetComboCount();
    if (bHasHitConnected && bHasHitStarted && !bCanCancelAfterHitboxEnds) bCanComboState = true;
    if (bHasHitStarted && bCanCancelAfterHitboxEnds) {
        IncComboCount();
        LastComboTime = GetWorld()->GetTimeSeconds();
        bHasHitConnected = true;
    }
    bHasHitStarted = false;
    ResetHitBoxes();
}

void UMyGameplayAbility::OnHitConnect(const FGameplayEventData Payload)
{
    if (!IsValid(GetAvatarActorFromActorInfo())) return;
    if (!bHasHitStarted) {
        // UE_LOG(LogTemp, Warning, TEXT("But has not started"));
        return;
    }
    IncComboCount();
    LastComboTime = GetWorld()->GetTimeSeconds();
    bHasHitConnected = true;
    CurHits++;
     UE_LOG(LogTemp, Warning, TEXT("Hit connected, CurHits: %d, MaxHits: %d"), CurHits, MaxHits);
     if (MaxHits && CurHits >= MaxHits)
     {
         UE_LOG(LogTemp, Warning, TEXT("true"));
         if (bSkipToRecovery) MontageJumpToSection(TEXT("Recovery"));
         else MontageSetNextSectionName(TEXT("Active"), TEXT("Recovery"));
     }
    IGetHit *Source = Cast<IGetHit>(GetAvatarActorFromActorInfo());
	if (!Source) return;
    Source->OnHitPause(HitPause);
    
}

void UMyGameplayAbility::ResetTarget()
{
    AMyCharacter* MyChar = Cast<AMyCharacter>(GetAvatarActorFromActorInfo());
    if (!MyChar) return;
    MyChar->SetTargetEnemy(nullptr);

}

/// <summary>
/// Acquires a new target from the character's target detection box. 
/// Finds the nearest player in the box and sets it as the character's "EnemyTarget"
/// Called in the beginning of the ability if bAqcuireNewTargetFromDetectionBox is true
/// </summary>
void UMyGameplayAbility::AcquireNewTarget()
{
    AMyCharacter* MyChar = Cast<AMyCharacter>(GetAvatarActorFromActorInfo());
    if (!MyChar) return;
    UPrimitiveComponent* Box = Cast<UPrimitiveComponent>(MyChar->TargetDetection);
    FTransform Tran = Box->GetComponentTransform();
    TArray < TEnumAsByte < EObjectTypeQuery > > ObjectTypes = TArray < TEnumAsByte < EObjectTypeQuery > >();
    //ObjectTypes.Add(TEnumAsByte < EObjectTypeQuery >(TestNum));
    ObjectTypes = TypesToTestTargetLock;
    //ObjectTypes = FCollisionObjectQueryParams(ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic));
    //ObjectTypes.Add(ECC_TO_BITFIELD(MyChar->GetCapsuleComponent()->GetCollisionObjectType()));
    TArray < AActor* > ActorsToIgnore = TArray < AActor* >();
    ActorsToIgnore.Add(GetAvatarActorFromActorInfo());
    TArray < class AActor* > OutActors;
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    UKismetSystemLibrary::ComponentOverlapActors(Box, Tran, ObjectTypes, AMyCharacter::StaticClass(), ActorsToIgnore, OutActors);
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    //MyChar->TargetDetection->GetOverlappingActors(OutActors, AMyCharacter::StaticClass());
    //UE_LOG(LogTemp, Warning, TEXT("Looking for overlapped chars"));
    AMyCharacter* Closest = nullptr;
    float Best = 9000.f;
    for (auto&& Overlapped : OutActors)
    {
        UE_LOG(LogTemp, Warning, TEXT("Overlapped: %s"), *Overlapped->GetName());
        AMyCharacter* OtherChar = Cast<AMyCharacter>(Overlapped);
        if (!OtherChar || !OtherChar->IsAlive()) continue;
        float Dist = FVector::Dist2D(MyChar->GetActorLocation(), OtherChar->GetActorLocation());
        if (Dist < Best)
        {
            Best = Dist;
            Closest = OtherChar;
        }
    }
    if (Closest)
    {
        MyChar->SetTargetEnemy(Closest);
    }
}

/// <summary>
/// Rotates the character towards its "EnemyTarget" variable 
/// called at the beginning of the ability if bLockRotationToTarget is true
/// </summary>
void UMyGameplayAbility::LockToTarget()
{
    UE_LOG(LogTemp, Warning, TEXT("Ability Locking to target"));
    AMyCharacter* MyChar = Cast<AMyCharacter>(GetAvatarActorFromActorInfo());
    if (!MyChar) return;
    
    AActor* EnemyTarget = MyChar->GetTargetEnemy();
    if (!EnemyTarget) return;
    FRotator NewRot = UKismetMathLibrary::FindLookAtRotation(MyChar->GetActorLocation(), EnemyTarget->GetActorLocation());
    NewRot.Pitch = MyChar->GetActorRotation().Pitch; NewRot.Roll = MyChar->GetActorRotation().Roll;
    MyChar->SetActorRotation(NewRot);
    //UE_LOG(LogTemp, Warning, TEXT("Rotated %s %s"), *MyChar->GetName(), *NewRot.ToCompactString());
}

float UMyGameplayAbility::GetAttackSpeed()
{
    AMyCharacter* MyChar = Cast<AMyCharacter>(GetAvatarActorFromActorInfo());
    if (!MyChar || !MyChar->GetAttributes()) return 1.0f;
    return MyChar->GetAttributes()->GetAttackSpeed() * MontagesSpeed;
}

/// <summary>
/// Applies all EffectsToApplyToSelf and store them in ActiveSelfEffects. Called when ability is just activated;
/// </summary>
void UMyGameplayAbility::ApplySelfEffects()
{
    ActiveSelfEffects.Append(UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToActor(GetAvatarActorFromActorInfo(), EffectsToApplyToSelf));
}

/// <summary>
/// Removes all effects in ActiveSelfEffects. Called when ability ends;
/// </summary>
void UMyGameplayAbility::RemoveSelfEffects()
{
    if (ActiveSelfEffects.Num() > 0)
    {
        UMyBlueprintFunctionLibrary::RemoveEffectsFromActor(GetAvatarActorFromActorInfo(), ActiveSelfEffects);
        ActiveSelfEffects = {};
    }
}

void UMyGameplayAbility::OnDeactivateEvent(const FGameplayEventData Payload)
{
    UE_LOG(LogTemp, Warning, TEXT("Deactivating ability"));
    //MontageJumpToSection(TEXT("Recovery"));
    MontageSetNextSectionName(TEXT("Active"), TEXT("Recovery"));
}

TArray<FGameplayEffectSpecHandle> UMyGameplayAbility::MakeSpecHandles()
{
    /*FGameplayTag HitStunTag = MyGameplayTags::Data_HitStun;
    FGameplayTag DamageTag = MyGameplayTags::Data_Damage;
    FGameplayTag KnockbackTag = MyGameplayTags::Data_Knockback;
    FGameplayTag LaunchTag = MyGameplayTags::Data_Launch;
    FGameplayTag LaunchXTag = MyGameplayTags::Data_Launch_X;
    FGameplayTag LaunchYTag = MyGameplayTags::Data_Launch_Y;
    FGameplayTag LaunchZTag = MyGameplayTags::Data_Launch_Z;*/
    TArray<FGameplayEffectSpecHandle> Result = {};
    auto EffectsToCheck = EffectsToApply;
    CheckConditionalEffects();
    EffectsToCheck.Append(TempEffectsToApply);
    for (auto &&Effect : EffectsToCheck)
    {
        if (!Effect.EffectClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: effect container without an effect class"), *GetName());
            continue;
        }
        FGameplayEffectSpecHandle NewHandle = MakeOutgoingGameplayEffectSpec(Effect.EffectClass);
        for (auto &&Mag : Effect.Magnitudes)
        {
            NewHandle.Data.Get()->SetSetByCallerMagnitude(Mag.GameplayTag, Mag.Magnitude);
        }
        
        Result.Add(NewHandle);
    }
    return Result;
}

// Check if any of the Conditional Effects and update temporary effects from results
void UMyGameplayAbility::CheckConditionalEffects()
{
    TempEffectsToApply.Empty();

    for (auto&& Condition : ConditionalEffects)
    {
        if (Condition.EffectToApply.EffectClass == nullptr || !Condition.ConditionTag.IsValid()) { continue; }
        // Needs an active effect that has the condition tag and also one of this ability's tags: Fire Hands
        // (GE_FirePunch: buff.firetouch + activate.punch) sets punches on fire, not kicks
        UAbilitySystemComponent* ASC = GetActorInfo().AbilitySystemComponent.Get();
        const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Condition.ConditionTag));
        for (const FActiveGameplayEffectHandle& Handle : ASC->GetActiveEffects(Query))
        {
            const FActiveGameplayEffect* Active = ASC->GetActiveGameplayEffect(Handle);
            FGameplayTagContainer EffectTags;
            if (Active) Active->Spec.GetAllAssetTags(EffectTags);
            if (EffectTags.HasAny(GetAssetTags()))
            {
                TempEffectsToApply.Add(Condition.EffectToApply);
                break;
            }
        }
    }

}

void UMyGameplayAbility::IncComboCount()
{
    bIsInComboState = true;
    FGameplayTag CanCancelState = MyGameplayTags::Combo_CanCancel;
    GetActorInfo().AbilitySystemComponent.Get()->AddLooseGameplayTag(CanCancelState);
    if (bHasHitConnected) return;
    if (CurrentComboCount + 1 < MontagesToPlay.Num()) ++CurrentComboCount;
    else ResetComboCount();
    // UE_LOG(LogTemp, Warning, TEXT("Increased combo to %d"), CurrentComboCount);
}

FGameplayTagContainer UMyGameplayAbility::GetAbilityTags()
{
    return GetAssetTags();
}

void UMyGameplayAbility::ResetComboCount()
{
    // UE_LOG(LogTemp, Warning, TEXT("Combo resetted"));
    CurrentComboCount = 0;

}

void UMyGameplayAbility::UpdateCombo()
{
    if (GetWorld()->GetTimeSeconds() > LastComboTime + ComboResetDelay) 
    {
        bIsInComboState = false;
        ResetComboCount();
    }
    // if (!bHasHitConnected || GetWorld()->GetTimeSeconds() > LastComboTime + ComboResetDelay) ResetCombo();
    // UE_LOG(LogTemp, Warning, TEXT("Updated combo to %d"), CurrentComboCount);
}

UClass* UMyGameplayAbility::GetHitBoxClass()
{
    UClass* Loaded = HitBoxClass.LoadSynchronous();
    return Loaded ? Loaded : AHitBox::StaticClass();
}

void UMyGameplayAbility::ResetHitBoxes()
{
    if (!IsValid(HitBoxRef) || !IsValid(GetAvatarActorFromActorInfo())) return;
    HitBoxRef->Destroy();
}
