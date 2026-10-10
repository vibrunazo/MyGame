// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayEffectTypes.h"
#include "IGetHit.generated.h"

enum class EHitReaction : uint8;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGetHit : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MYGAME_API IGetHit
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// Applies an effect to me (hits, items, an ability's self effects); returns the active handle for duration effects
	virtual FActiveGameplayEffectHandle OnGetHitByEffect(FGameplayEffectSpecHandle NewEffect) = 0;
	// An applied effect's UHitReactionEffectComponent asks for a reaction; the attacker is the spec's effect causer
	virtual void OnHitReaction(EHitReaction Reaction, const FGameplayEffectSpec& Spec) = 0;
	virtual void OnDamaged(AActor* SourceActor, float Damage, FGameplayEffectSpec Effect) = 0;
	virtual void OnDie() = 0;
	virtual bool IsAlive() = 0;
	virtual void OnHitPause(float Duration) = 0;
	// virtual void OnSpeedChange() = 0;
	virtual uint8 GetTeam() = 0;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const = 0;
	virtual class UMyAttributeSet* GetAttributes() = 0;
	virtual void AddItemToInventory(class UItemDataAsset* NewItem) = 0;
	virtual bool IsProjectileImmune() = 0;
	// virtual void ApplyKnockBack(FVector Dir) = 0;
};
