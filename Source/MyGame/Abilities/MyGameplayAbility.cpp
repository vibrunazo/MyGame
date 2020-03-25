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
#include "IGetHit.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"

UMyGameplayAbility::UMyGameplayAbility()
{
    static ConstructorHelpers::FClassFinder<AActor> HitBoxClassFinder(TEXT("/Game/Blueprints/BP_HitBox"));
    HitBoxClass = HitBoxClassFinder.Class;
}

bool UMyGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const
{
    FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(TEXT("state.attacking"));
    if(ActorInfo->AbilitySystemComponent.Get()->HasMatchingGameplayTag(AttackTag))
    {
        if (bHasHitConnected && GetWorld()->GetTimeSeconds() > LastComboTime + HitToComboDelay) 
        {
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
    ResetHitBoxes();
    UpdateCombo();
    bHasHitConnected = false;
    bHasHitStarted = false;
    UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontagesToPlay[CurrentComboCount], 1.0f, NAME_None, false, 1.0f);
    Task->OnCompleted.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->OnInterrupted.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->OnCancelled.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->OnBlendOut.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->ReadyForActivation();

    FGameplayTag HitStartTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.start"));;
    FGameplayTag HitEndTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.end"));
    FGameplayTag HitConnectTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.connect"));

    UAbilityTask_WaitGameplayEvent* HitStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitStartTag);
    HitStartTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitStart);
    HitStartTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* HitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitEndTag);
    HitEndTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitEnd);
    HitEndTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* HitConnectTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitConnectTag);
    HitConnectTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitConnect);
    HitConnectTask->ReadyForActivation();
    
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMyGameplayAbility::OnMontageComplete()
{
    bHasHitConnected = false;
    bHasHitStarted = false;
    ResetHitBoxes();
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UMyGameplayAbility::OnHitStart(const FGameplayEventData Payload)
{
    ResetHitBoxes();
    bHasHitStarted = true;
    // UE_LOG(LogTemp, Warning, TEXT("Hit started"));
    // TODO might crash if I'm dead?
    FVector Loc = CurrentActorInfo->AvatarActor->GetActorLocation();
    FActorSpawnParameters params;
    params.bNoFail = true;
    params.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
    params.Owner = GetAvatarActorFromActorInfo();
    AHitBox* NewActor = GetWorld()->SpawnActor<AHitBox>(HitBoxClass, Loc, FRotator::ZeroRotator, params);
    NewActor->AttachToActor(GetAvatarActorFromActorInfo(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    NewActor->EffectsToApply = MakeSpecHandles();
    // const UHitboxSettings* Settings = Cast<UHitboxSettings>(&Payload.OptionalObject);
    const UObject* OO = Payload.OptionalObject;
    if (OO) {
        UHitboxSettings* Settings = (UHitboxSettings*)(Payload.OptionalObject);
        if (!ensure(Settings != nullptr)) return;
        NewActor->AddComponentsToBones(Settings->BoneNames);
    }
    // else {
    // }

    HitBoxRef = NewActor;
}

void UMyGameplayAbility::OnHitEnd(const FGameplayEventData Payload)
{
    // UE_LOG(LogTemp, Warning, TEXT("Hit ended"));
    if (!bHasHitConnected && bHasHitStarted) ResetCombo();
    bHasHitStarted = false;
    ResetHitBoxes();
}

void UMyGameplayAbility::OnHitConnect(const FGameplayEventData Payload)
{
    // UE_LOG(LogTemp, Warning, TEXT("Hit connected"));
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

TArray<FGameplayEffectSpecHandle> UMyGameplayAbility::MakeSpecHandles()
{
    FGameplayTag HitStunTag = FGameplayTag::RequestGameplayTag(TEXT("data.hitstun"));
    FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(TEXT("data.damage"));
    TArray<FGameplayEffectSpecHandle> Result = {};
    for (auto &&Effect : EffectsToApply)
    {
        FGameplayEffectSpecHandle NewHandle = MakeOutgoingGameplayEffectSpec(Effect);
        FGameplayTagContainer EffectTags;
        NewHandle.Data.Get()->GetAllAssetTags(EffectTags);
        if (EffectTags.HasTag(HitStunTag))
        {
            NewHandle.Data.Get()->SetSetByCallerMagnitude(HitStunTag, HitStun);
        }
        if (EffectTags.HasTag(DamageTag))
        {
            NewHandle.Data.Get()->SetSetByCallerMagnitude(DamageTag, -Damage);
        }
        // EffectTags.HasTag(HitStunTag);
        // UE_LOG(LogTemp, Warning, TEXT("Effect: %s, tags: %s, has hitstun: %d"), *Effect.Get()->GetName(), *Container.ToString(), Container.HasTag(HitStunTag));
        // NewHandle.Data.Get()->GetAllAssetTags();
        // NewHandle.Data.Get()->SetSetByCallerMagnitude();
        Result.Add(NewHandle);
    }
    return Result;
}

void UMyGameplayAbility::IncComboCount()
{
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
    if (GetWorld()->GetTimeSeconds() > LastComboTime + ComboResetDelay) ResetCombo();
    // if (!bHasHitConnected || GetWorld()->GetTimeSeconds() > LastComboTime + ComboResetDelay) ResetCombo();
    // UE_LOG(LogTemp, Warning, TEXT("Updated combo to %d"), CurrentComboCount);
}

void UMyGameplayAbility::ResetHitBoxes()
{
    if (!HitBoxRef) return;
    HitBoxRef->Destroy();
}
