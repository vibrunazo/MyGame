// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayEffectTypes.h"
#include "GetHit.generated.h"

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
	virtual void OnGetHitByEffect(FGameplayEffectSpecHandle NewEffect) = 0;
	virtual void OnDamaged(AActor* SourceActor) = 0;
	virtual void OnDie() = 0;
	virtual bool IsAlive() = 0;
	virtual void OnHitPause(float Duration) = 0;
	virtual uint8 GetTeam() = 0;
};
