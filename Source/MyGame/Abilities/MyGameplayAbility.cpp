// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameplayAbility.h"

void UMyGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo * ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData * TriggerEventData)
{
    UE_LOG(LogTemp, Warning, TEXT("Activate Ability from cpp"));
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}