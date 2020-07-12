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
	AEnemyCharBase();
	void SetDefaultProperties() override;
	void BeginPlay() override;

	void OnDamaged(AActor* SourceActor) override;

	void SetOutline();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Base")
	float OutlineRemoveDelay = 3.5f;

protected:
	FTimerHandle OutlineTimer;
};
