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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	class USoundBase* HitSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	class USoundBase* BlockSound;
	// TArray<TSubclassOf<class UGameplayEffect>> EffectsToApply;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float HitPause = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	bool bCanCombo = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float HitToComboDelay = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float ComboResetDelay = 2.00f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FGameplayTagContainer TagsIcanCancel;
	// if true, I can only cancel abilities with TagsIcanCancel if the last ability hits, so I can combo from the hit, if false then always cancel it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bNeedsHitToCancel = true;
	// if true, at the beginning of this ability, rotate the player towards the direction of controller input
	// this will garantee that slow mesh rotation won't make the ability not target where the player is trying to point to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bUpdateRotationFromController = true;
	// if true, at the beginning of this ability, will reset the current target before acquiring a new one
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bResetTarget = false;
	// if true, at the beginning of this ability, rotate towards the current target and prevents player from changing rotation until ability ends
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bLockRotationToTarget = false;
	// if true, at the beginning of this ability, acquires a new target: the nearest target that overlaps the target detection box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bAqcuireNewTargetFromDetectionBox = false;
	// If true, activating this ability puts the user in Combat
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bStartsCombat = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TArray < TEnumAsByte < EObjectTypeQuery > > TypesToTestTargetLock;

private:
	UPROPERTY()
	uint8 CurrentComboCount = 0;
	UPROPERTY()
	bool bHasHitConnected = false;
	UPROPERTY()
	bool bCanComboState = false;
	UPROPERTY()
	bool bIsInComboState = false;
	void ResetComboCount();
	void UpdateCombo();
	UPROPERTY()
	float LastComboTime = 0.0f;
	UPROPERTY()
	bool bHasHitStarted = false;

protected:
/** Returns true if this ability can be activated right now. Has no side effects */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo * ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData * TriggerEventData) override;
	/** Native function, called if an ability ends normally or abnormally. If bReplicate is set to true, try to replicate the ending to the client/server */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
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
	void ResetTarget(); 
	void AcquireNewTarget();
	void LockToTarget();

	UPROPERTY()
	TSubclassOf<class AHitBox> HitBoxClass;
	UPROPERTY()
	class AHitBox* HitBoxRef = nullptr;
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveEffects = {};
	UPROPERTY()
	FRotator InitialRotRate = FRotator();
	
};
