// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MyGameplayEffectUIData.h"
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
	UFUNCTION(BlueprintImplementableEvent, Category = Abilities)
	void BPOnAbilityActivated(UMyGameplayAbility* ActivatedAbility);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FGameplayTagContainer GetAbilityTags();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Ability")
	TArray<FConditionalEffect> ConditionalEffects;
	UPROPERTY()
	// Temporary effects to apply, when some conditional effect is triggered
	TArray<FEffectContainer> TempEffectsToApply;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TArray<UAnimMontage*> MontagesToPlay;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float MontagesSpeed = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	FBuffUI AbilityUIData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TArray<FEffectContainer> EffectsToApply;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	class USoundBase* HitSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	class USoundBase* BlockSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class UNiagaraSystem* HitParticles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class UNiagaraSystem* BlockParticles;
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
	// if true, I can cancel abilities with TagsIcanCancel if the last ability hits, so I can combo from the hit, if false then always cancel it
	// if false, abilities can cancel at any time. Which might cause button spam to cancel itself.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bNeedsHitToCancel = true;
	// if true, I can also cancel abilities after the hitbox ends with the notify.hit.end tag, regardless if it hits or not
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bCanCancelAfterHitboxEnds = false;
	// if true, at the beginning of this ability, rotate the player towards the direction of controller input
	// this will garantee that slow mesh rotation won't make the ability not target where the player is trying to point to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bUpdateRotationFromController = true;
	// if true, at the beginning of this ability, will reset the current target before acquiring a new one
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bResetTarget = false;
	// if true, at the beginning of this ability, rotate towards the current target and IF there's a target, prevents player from changing rotation until ability ends
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bLockRotationToTarget = false;
	// if true, at the beginning of this ability, acquires a new target: the nearest target that overlaps the target detection box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bAqcuireNewTargetFromDetectionBox = false;
	// if true, lock rotation regardless of targetting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bAlwaysLockRot = false;
	// If true, activating this ability puts the user in Combat
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bStartsCombat = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TArray < TEnumAsByte < EObjectTypeQuery > > TypesToTestTargetLock;
	// When this gameplay event tag is fired on this Actor, this ability should cancel its active section of the montage and go into recovery
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FGameplayTag TagThatDeactivateMe;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	// If > 0, will change montage section to Recovery as soon as the ability hits enemies this many times
	int MaxHits = 0;

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
	// How many times this ability already hit the enemy since it started, used to deactivate the ability if it has a set limit of MaxHits
	int CurHits = 0;

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
	UFUNCTION()
	void OnDeactivateEvent(const FGameplayEventData Payload);
	TArray<struct FGameplayEffectSpecHandle> MakeSpecHandles();
	void CheckConditionalEffects();
	void IncComboCount();
	void ResetHitBoxes();
	void ResetActiveEffects();
	void ResetTarget(); 
	void AcquireNewTarget();
	void LockToTarget();
	void LockRot();
	float GetAttackSpeed();

	UPROPERTY()
	TSubclassOf<class AHitBox> HitBoxClass;
	UPROPERTY()
	class AHitBox* HitBoxRef = nullptr;
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveEffects = {};
	UPROPERTY()
	FRotator InitialRotRate = FRotator();
	
};
