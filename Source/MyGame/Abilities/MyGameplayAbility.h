// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MyGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UMyGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UMyGameplayAbility();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TArray<UAnimMontage*> MontagesToPlay;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TArray<TSubclassOf<class UGameplayEffect>> EffectsToApply;
	bool bHasHitConnected = false;
	bool bCanCancelIntoCombo = false;
	uint8 CurrentComboCount = 0;
private:
	void ResetCombo();
	void UpdateCombo();
	float LastComboTime = 0.0f;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo * ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData * TriggerEventData);
	UFUNCTION()
	void OnMontageComplete();
	UFUNCTION()
	void OnHitStart(const FGameplayEventData Payload);
	UFUNCTION()
	void OnHitEnd(const FGameplayEventData Payload);
	UFUNCTION()
	void OnHitConnect(const FGameplayEventData Payload);
	TArray<struct FGameplayEffectSpecHandle> MakeSpecHandles();
	void IncComboCount();

	TSubclassOf<class AHitBox> HitBoxClass;
	class AHitBox* HitBoxRef;

	
};
