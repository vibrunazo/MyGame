// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "../MyBlueprintFunctionLibrary.h"
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
	TArray<FEffectContainer> EffectsToApply;
	// TArray<TSubclassOf<class UGameplayEffect>> EffectsToApply;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float HitToComboDelay = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float ComboResetDelay = 2.00f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float Damage = 15.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float HitStun = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float HitPause = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float KnockBack = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FVector LaunchVector = FVector(200.0f, 0.0f, 300.0f);
	uint8 CurrentComboCount = 0;
	bool bHasHitConnected = false;
private:
	void ResetCombo();
	void UpdateCombo();
	float LastComboTime = 0.0f;
	bool bHasHitStarted = false;

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
	UFUNCTION()
	void OnEffectApplyEvent(const FGameplayEventData Payload);
	UFUNCTION()
	void OnEffectRemoveEvent(const FGameplayEventData Payload);
	TArray<struct FGameplayEffectSpecHandle> MakeSpecHandles();
	void IncComboCount();
	void ResetHitBoxes();
	void ResetActiveEffects();
	TSubclassOf<class AHitBox> HitBoxClass;
	class AHitBox* HitBoxRef = nullptr;
	TArray<FActiveGameplayEffectHandle> ActiveEffects = {};

	
};
