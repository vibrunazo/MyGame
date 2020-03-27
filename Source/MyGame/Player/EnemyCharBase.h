// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyCharacter.h"
#include "EnemyCharBase.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API AEnemyCharBase : public AMyCharacter
{
	GENERATED_BODY()
public:
	void SetDefaultProperties() override;
};
