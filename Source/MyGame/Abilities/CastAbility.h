// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyGameplayAbility.h"
#include "CastAbility.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UCastAbility : public UMyGameplayAbility
{
	GENERATED_BODY()

public:
	// UCastAbility();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TSubclassOf<class AMyProjectile> ProjectileToSpawn;

protected:
	void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo * ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData * TriggerEventData) override;
	UFUNCTION()
	void OnCast(const FGameplayEventData Payload);
	
};
