// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameplayAbility.h"
#include "IGetHit.h"
#include "../Player/MyCharacter.h"
#include "../Player/HitBox.h"
#include "../Player/HitboxSettings.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
// #include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "UObject/ConstructorHelpers.h"
#include "GameplayEffect.h"
#include "EffectEventSettings.h"
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
    static ConstructorHelpers::FClassFinder<AActor> HitBoxClassFinder(TEXT("/Game/Blueprints/Chars/BP_HitBox"));
    HitBoxClass = HitBoxClassFinder.Class;
}

bool UMyGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const
{
    //UE_LOG(LogTemp, Warning, TEXT("Trying Ability: %s on %s"), *GetName(), *GetAvatarActorFromActorInfo()->GetName());
    FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(TEXT("state.attacking"));
    // This tag should be used when the ability is in a State where other abilties can cancel its animation to combo into some other ability
    FGameplayTag CanCancelState = FGameplayTag::RequestGameplayTag(TEXT("combo.cancancel"));
    // This tag will be set as soon as an ability is cancelled into another, so that the next ability knows that it's starting from a cancel, so that it can move the montage section to ComboStart
    FGameplayTag IsCancellingState = FGameplayTag::RequestGameplayTag(TEXT("combo.iscancelling"));
    // If I'm in the middle of an attack
    if(ActorInfo->AbilitySystemComponent.Get()->HasMatchingGameplayTag(AttackTag))
    {
        // check if I can cancel an attack of the current type
        if (ActorInfo->AbilitySystemComponent.Get()->HasAnyMatchingGameplayTags(TagsIcanCancel))
        {
            // check if I need a combo hit to cancel and if I have hit, by checking if the CanCancelState Tag was applied by any ability
            if (bNeedsHitToCancel && ActorInfo->AbilitySystemComponent.Get()->HasMatchingGameplayTag(CanCancelState))
            {
                // Add tag to actor to let the next ability know it's comming from a cancelled ability
                // TODO check if super is true before doing this?
                ActorInfo->AbilitySystemComponent.Get()->AddLooseGameplayTag(IsCancellingState);
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
    FName MontageSection = NAME_None;
    //if (bIsInComboState) MontageSection = "ComboStart";
    FGameplayTag CanCancelState = FGameplayTag::RequestGameplayTag(TEXT("combo.cancancel"));
    FGameplayTag IsCancellingState = FGameplayTag::RequestGameplayTag(TEXT("combo.iscancelling"));
    if (ActorInfo->AbilitySystemComponent.Get()->HasMatchingGameplayTag(IsCancellingState) && bNeedsHitToCancel)
    {
        MontageSection = "ComboStart";
    }
    GetActorInfo().AbilitySystemComponent.Get()->RemoveLooseGameplayTag(CanCancelState);
    GetActorInfo().AbilitySystemComponent.Get()->RemoveLooseGameplayTag(IsCancellingState);
    UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontagesToPlay[CurrentComboCount], GetAttackSpeed(), MontageSection, false, 1.0f);
    Task->OnCompleted.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->OnInterrupted.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->OnCancelled.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->OnBlendOut.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->ReadyForActivation();

    FGameplayTag HitStartTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.start"));;
    FGameplayTag HitEndTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.end"));
    FGameplayTag HitConnectTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.connect"));
    FGameplayTag EffectApplyTag = FGameplayTag::RequestGameplayTag(TEXT("notify.effect.apply"));
    FGameplayTag EffectRemoveTag = FGameplayTag::RequestGameplayTag(TEXT("notify.effect.remove"));

    UAbilityTask_WaitGameplayEvent* HitStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitStartTag);
    HitStartTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitStart);
    HitStartTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* HitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitEndTag);
    HitEndTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitEnd);
    HitEndTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* HitConnectTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitConnectTag);
    HitConnectTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitConnect);
    HitConnectTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* EffectApplyTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EffectApplyTag);
    EffectApplyTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnEffectApplyEvent);
    EffectApplyTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* EffectRemoveTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EffectRemoveTag);
    EffectRemoveTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnEffectRemoveEvent);
    EffectRemoveTask->ReadyForActivation();
    
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMyGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (!bLockRotationToTarget) return;
    AMyCharacter* MyChar = Cast<AMyCharacter>(GetAvatarActorFromActorInfo());
    if (!MyChar) return;
    UCharacterMovementComponent* Move = Cast<UCharacterMovementComponent>(MyChar->GetMovementComponent());
    if (Move) Move->RotationRate = InitialRotRate;
    UE_LOG(LogTemp, Warning, TEXT("EndAbility, set RotationRate to %s"), *InitialRotRate.ToString());

}

void UMyGameplayAbility::OnMontageComplete()
{
    bHasHitConnected = false;
    bHasHitStarted = false;
    if (!IsValid(GetAvatarActorFromActorInfo())) return;
    ResetHitBoxes();
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
    ResetActiveEffects();
    FGameplayTag CanCancelState = FGameplayTag::RequestGameplayTag(TEXT("combo.cancancel"));
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
    AHitBox* NewHB = GetWorld()->SpawnActor<AHitBox>(HitBoxClass, Loc, FRotator::ZeroRotator, params);
    NewHB->AttachToActor(GetAvatarActorFromActorInfo(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    NewHB->HitSound = HitSound;
    NewHB->BlockSound = BlockSound;
    NewHB->HitParticles = HitParticles;
    NewHB->BlockParticles = BlockParticles;
    NewHB->EffectsToApply = MakeSpecHandles();
    // const UHitboxSettings* Settings = Cast<UHitboxSettings>(&Payload.OptionalObject);
    const UObject* OO = Payload.OptionalObject;
    if (OO) {
        UHitboxesContainer* Settings = (UHitboxesContainer*)(Payload.OptionalObject);
        if (!ensure(Settings != nullptr)) return;
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
     //UE_LOG(LogTemp, Warning, TEXT("Hit connected"));
    if (!IsValid(GetAvatarActorFromActorInfo())) return;
    if (!bHasHitStarted) {
        // UE_LOG(LogTemp, Warning, TEXT("But has not started"));
        return;
    }
    IncComboCount();
    LastComboTime = GetWorld()->GetTimeSeconds();
    bHasHitConnected = true;
    IGetHit *Source = Cast<IGetHit>(GetAvatarActorFromActorInfo());
	if (!Source) return;
    Source->OnHitPause(HitPause);
    
}

void UMyGameplayAbility::OnEffectApplyEvent(const FGameplayEventData Payload)
{
     //if (((AMyCharacter*)GetAvatarActorFromActorInfo())->IsPlayerControlled()) UE_LOG(LogTemp, Warning, TEXT("Ability receied effect apply event"));
    UE_LOG(LogTemp, Warning, TEXT("Ability receied effect apply event"));
    //ResetActiveEffects();
    const UEffectEventSettings* Settings = Cast<UEffectEventSettings>(Payload.OptionalObject);
    if (Settings)
    {
        ActiveEffects.Append(UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToActor(GetAvatarActorFromActorInfo(), Settings->EffectsToApply));
    }
}
void UMyGameplayAbility::OnEffectRemoveEvent(const FGameplayEventData Payload)
{
    ResetActiveEffects();
}

void UMyGameplayAbility::ResetActiveEffects()
{
    if (((AMyCharacter*)GetAvatarActorFromActorInfo())->IsPlayerControlled()) UE_LOG(LogTemp, Warning, TEXT("Ability received effect remove event"));
    if (ActiveEffects.Num() > 0)
    {
        UMyBlueprintFunctionLibrary::RemoveEffectsFromActor(GetAvatarActorFromActorInfo(), ActiveEffects);
        ActiveEffects = {};
    }
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
/// and locks rotation rate to zero, (EndAbility should reset this)
/// called at the beginning of the ability if bLockRotationToTarget is trye
/// </summary>
void UMyGameplayAbility::LockToTarget()
{
    UE_LOG(LogTemp, Warning, TEXT("Ability Locking to target"));
    AMyCharacter* MyChar = Cast<AMyCharacter>(GetAvatarActorFromActorInfo());
    if (!MyChar) return;
    InitialRotRate = FRotator(0.f, 600.f, 0.f);
    AActor* EnemyTarget = MyChar->GetTargetEnemy();
    if (!EnemyTarget) return;
    FRotator NewRot = UKismetMathLibrary::FindLookAtRotation(MyChar->GetActorLocation(), EnemyTarget->GetActorLocation());
    NewRot.Pitch = MyChar->GetActorRotation().Pitch; NewRot.Roll = MyChar->GetActorRotation().Roll;
    MyChar->SetActorRotation(NewRot);
    //UE_LOG(LogTemp, Warning, TEXT("Rotated %s %s"), *MyChar->GetName(), *NewRot.ToCompactString());
    UCharacterMovementComponent* Move = Cast<UCharacterMovementComponent>(MyChar->GetMovementComponent());
    if (Move)
    {
        // TODO should not be hard coded, but getting initial rot from movement component was failing sometimes
        // should get it from the character's walk rotation variables
        //InitialRotRate = Move->RotationRate;
        Move->RotationRate = FRotator(0.f, 0.f, 0.f);
    }
}

float UMyGameplayAbility::GetAttackSpeed()
{
    AMyCharacter* MyChar = Cast<AMyCharacter>(GetAvatarActorFromActorInfo());
    if (!MyChar || !MyChar->GetAttributes()) return 1.0f;
    return MyChar->GetAttributes()->GetAttackSpeed() * MontagesSpeed;
}

TArray<FGameplayEffectSpecHandle> UMyGameplayAbility::MakeSpecHandles()
{
    /*FGameplayTag HitStunTag = FGameplayTag::RequestGameplayTag(TEXT("data.hitstun"));
    FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(TEXT("data.damage"));
    FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("data.knockback"));
    FGameplayTag LaunchTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch"));
    FGameplayTag LaunchXTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.x"));
    FGameplayTag LaunchYTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.y"));
    FGameplayTag LaunchZTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.z"));*/
    TArray<FGameplayEffectSpecHandle> Result = {};
    auto EffectsToCheck = EffectsToApply;
    CheckConditionalEffects();
    EffectsToCheck.Append(TempEffectsToApply);
    for (auto &&Effect : EffectsToCheck)
    {
        if (!Effect.EffectClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("no class"));
            break;
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
        if (Condition.EffectToApply.EffectClass == nullptr) { continue; }
        auto MyTags = GetAbilityTags();
        auto EffectsThatMatchAbility = GetActorInfo().AbilitySystemComponent.Get()->GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(MyTags));
        if (EffectsThatMatchAbility.Num() > 0) TempEffectsToApply.Add(Condition.EffectToApply);
    }

}

void UMyGameplayAbility::IncComboCount()
{
    bIsInComboState = true;
    FGameplayTag CanCancelState = FGameplayTag::RequestGameplayTag(TEXT("combo.cancancel"));
    GetActorInfo().AbilitySystemComponent.Get()->AddLooseGameplayTag(CanCancelState);
    if (bHasHitConnected) return;
    if (CurrentComboCount + 1 < MontagesToPlay.Num()) ++CurrentComboCount;
    else ResetComboCount();
    // UE_LOG(LogTemp, Warning, TEXT("Increased combo to %d"), CurrentComboCount);
}

FGameplayTagContainer UMyGameplayAbility::GetAbilityTags()
{
    return AbilityTags;
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

void UMyGameplayAbility::ResetHitBoxes()
{
    if (!HitBoxRef || !HitBoxRef->IsValidLowLevel() || !GetAvatarActorFromActorInfo() || !GetAvatarActorFromActorInfo()->IsValidLowLevel()) return;
    HitBoxRef->Destroy();
}
