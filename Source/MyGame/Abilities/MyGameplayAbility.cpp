// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
// #include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "UObject/ConstructorHelpers.h"
#include "GameplayEffect.h"
#include "../Player/HitBox.h"
#include "../Player/HitboxSettings.h"
#include "EffectEventSettings.h"
// #include "../MyBlueprintFunctionLibrary.h"
#include "IGetHit.h"
#include "../Player/MyCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"

UMyGameplayAbility::UMyGameplayAbility()
{
    static ConstructorHelpers::FClassFinder<AActor> HitBoxClassFinder(TEXT("/Game/Blueprints/Chars/BP_HitBox"));
    HitBoxClass = HitBoxClassFinder.Class;
}

bool UMyGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const
{
    FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(TEXT("state.attacking"));
    // If I'm in the middle of an attack
    if(ActorInfo->AbilitySystemComponent.Get()->HasMatchingGameplayTag(AttackTag))
    {
        // check if it can be cancelled into a combo
        if (bCanCombo && bHasHitConnected && (GetWorld()->GetTimeSeconds() > LastComboTime + HitToComboDelay || bCanComboState)) 
        {
            // it can be cancelled so call Super to do regular checks if I can cast this ability 
            return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
        }
        return false;
    }
    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UMyGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo * ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData * TriggerEventData)
{
    CommitAbility(Handle, ActorInfo, ActivationInfo);
    if (MontagesToPlay.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No Montages in Ability"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
        return;
    }
    if (!IsValid(GetAvatarActorFromActorInfo())) return;
    // ResetHitBoxes();
    UpdateCombo();
    bHasHitConnected = false;
    bCanComboState = false;
    bHasHitStarted = false;
    // ActorInfo->AvatarActor()->
    FName MontageSection = NAME_None;
    if (bIsInComboState) MontageSection = "ComboStart";
    UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontagesToPlay[CurrentComboCount], 1.0f, MontageSection, false, 1.0f);
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

void UMyGameplayAbility::OnMontageComplete()
{
    bHasHitConnected = false;
    bHasHitStarted = false;
    if (!IsValid(GetAvatarActorFromActorInfo())) return;
    ResetHitBoxes();
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
    ResetActiveEffects();
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
    NewHB->EffectsToApply = MakeSpecHandles();
    // const UHitboxSettings* Settings = Cast<UHitboxSettings>(&Payload.OptionalObject);
    const UObject* OO = Payload.OptionalObject;
    if (OO) {
        UHitboxSettings* Settings = (UHitboxSettings*)(Payload.OptionalObject);
        if (!ensure(Settings != nullptr)) return;
        NewHB->SphereRadius = Settings->SphereRadius;
        NewHB->AddComponentsToBones(Settings->BoneNames);
    }

    HitBoxRef = NewHB;
}

void UMyGameplayAbility::OnHitEnd(const FGameplayEventData Payload)
{
    // UE_LOG(LogTemp, Warning, TEXT("Hit ended"));
    if (!IsValid(GetAvatarActorFromActorInfo())) return;
    if (!bHasHitConnected && bHasHitStarted) ResetCombo();
    if (bHasHitConnected && bHasHitStarted) bCanComboState = true;
    bHasHitStarted = false;
    ResetHitBoxes();
}

void UMyGameplayAbility::OnHitConnect(const FGameplayEventData Payload)
{
    // UE_LOG(LogTemp, Warning, TEXT("Hit connected"));
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
    // if (((AMyCharacter*)GetAvatarActorFromActorInfo())->IsPlayerControlled()) UE_LOG(LogTemp, Warning, TEXT("Ability received effect apply event"));
    ResetActiveEffects();
    const UEffectEventSettings* Settings = Cast<UEffectEventSettings>(Payload.OptionalObject);
    if (Settings)
    {
        ActiveEffects = UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToActor(GetAvatarActorFromActorInfo(), Settings->EffectsToApply);
    }
}
void UMyGameplayAbility::OnEffectRemoveEvent(const FGameplayEventData Payload)
{
    ResetActiveEffects();
}

void UMyGameplayAbility::ResetActiveEffects()
{
    // if (((AMyCharacter*)GetAvatarActorFromActorInfo())->IsPlayerControlled()) UE_LOG(LogTemp, Warning, TEXT("Ability received effect remove event"));
    if (ActiveEffects.Num() > 0)
    {
        UMyBlueprintFunctionLibrary::RemoveEffectsFromActor(GetAvatarActorFromActorInfo(), ActiveEffects);
        ActiveEffects = {};
    }
}

TArray<FGameplayEffectSpecHandle> UMyGameplayAbility::MakeSpecHandles()
{
    FGameplayTag HitStunTag = FGameplayTag::RequestGameplayTag(TEXT("data.hitstun"));
    FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(TEXT("data.damage"));
    FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("data.knockback"));
    FGameplayTag LaunchTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch"));
    FGameplayTag LaunchXTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.x"));
    FGameplayTag LaunchYTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.y"));
    FGameplayTag LaunchZTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.z"));
    TArray<FGameplayEffectSpecHandle> Result = {};
    for (auto &&Effect : EffectsToApply)
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

void UMyGameplayAbility::IncComboCount()
{
    bIsInComboState = true;
    if (bHasHitConnected) return;
    if (CurrentComboCount + 1 < MontagesToPlay.Num()) ++CurrentComboCount;
    else ResetCombo();
    // UE_LOG(LogTemp, Warning, TEXT("Increased combo to %d"), CurrentComboCount);
}

void UMyGameplayAbility::ResetCombo()
{
    // UE_LOG(LogTemp, Warning, TEXT("Combo resetted"));
    CurrentComboCount = 0;

}

void UMyGameplayAbility::UpdateCombo()
{
    if (GetWorld()->GetTimeSeconds() > LastComboTime + ComboResetDelay) 
    {
        bIsInComboState = false;
        ResetCombo();
    }
    // if (!bHasHitConnected || GetWorld()->GetTimeSeconds() > LastComboTime + ComboResetDelay) ResetCombo();
    // UE_LOG(LogTemp, Warning, TEXT("Updated combo to %d"), CurrentComboCount);
}

void UMyGameplayAbility::ResetHitBoxes()
{
    if (!HitBoxRef || !HitBoxRef->IsValidLowLevel() || !GetAvatarActorFromActorInfo() || !GetAvatarActorFromActorInfo()->IsValidLowLevel()) return;
    HitBoxRef->Destroy();
}
