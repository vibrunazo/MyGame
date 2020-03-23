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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float HitToComboDelay = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float ComboResetDelay = 2.00f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float HitPause = 0.50f;
	bool bHasHitConnected = false;
	uint8 CurrentComboCount = 0;
private:
	void ResetCombo();
	void UpdateCombo();
	float LastComboTime = 0.0f;

protected:
/** Returns true if this ability can be activated right now. Has no side effects */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo * ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData * TriggerEventData) override;
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
