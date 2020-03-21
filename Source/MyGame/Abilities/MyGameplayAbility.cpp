// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "UObject/ConstructorHelpers.h"
#include "../Player/HitBox.h"

UMyGameplayAbility::UMyGameplayAbility()
{
    static ConstructorHelpers::FClassFinder<AActor> HitBoxClassFinder(TEXT("/Game/Blueprints/BP_HitBox"));
    HitBoxClass = HitBoxClassFinder.Class;
}

void UMyGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo * ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData * TriggerEventData)
{
    UE_LOG(LogTemp, Warning, TEXT("Activate Ability from cpp"));
    CommitAbility(Handle, ActorInfo, ActivationInfo);
    UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay, 1.0f, NAME_None, false, 1.0f);
    Task->OnCompleted.AddDynamic(this, &UMyGameplayAbility::OnMontageComplete);
    Task->ReadyForActivation();

    FGameplayTag HitStartTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.start"));;
    FGameplayTag HitEndTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.end"));

    UAbilityTask_WaitGameplayEvent* HitStartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitStartTag);
    HitStartTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitStart);
    HitStartTask->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* HitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitEndTag);
    HitEndTask->EventReceived.AddDynamic(this, &UMyGameplayAbility::OnHitEnd);
    HitEndTask->ReadyForActivation();
    
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMyGameplayAbility::OnMontageComplete()
{
    // UE_LOG(LogTemp, Warning, TEXT("Montage Complete"));
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UMyGameplayAbility::OnHitStart(const FGameplayEventData Payload)
{
    UE_LOG(LogTemp, Warning, TEXT("Hit start"));
    FVector Loc = CurrentActorInfo->AvatarActor->GetActorLocation();
    AHitBox* NewActor = GetWorld()->SpawnActor<AHitBox>(HitBoxClass, Loc, FRotator::ZeroRotator);  
    HitBoxRef = NewActor;
}

void UMyGameplayAbility::OnHitEnd(const FGameplayEventData Payload)
{
    UE_LOG(LogTemp, Warning, TEXT("Hit end"));
    HitBoxRef->Destroy();
}