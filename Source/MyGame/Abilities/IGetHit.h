// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayEffectTypes.h"
#include "IGetHit.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGetHit : public UInterface
{
	GENERATED_BODY()
};

/**
 * MyGame's side of a character: damage and death (its attribute set calls these until Step 11), its attributes and its
 * loot. What combat asks of a character is IAssCombatant (Ass Plan.md 4.1).
 */
class MYGAME_API IGetHit
{
	GENERATED_BODY()

public:
	virtual void OnDamaged(AActor* SourceActor, float Damage, FGameplayEffectSpec Effect) = 0;
	virtual void OnDie() = 0;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const = 0;
	virtual class UMyAttributeSet* GetAttributes() = 0;
	virtual void AddItemToInventory(class UItemDataAsset* NewItem) = 0;
};
