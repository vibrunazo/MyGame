// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "MyAIController.generated.h"

/**
 * Base for the enemies' AI controllers. Sight is AI Perception (one sight sense); noticing the player hands it to
 * the character's OnPawnSeen, which sets the blackboard target and aggroes the room.
 */
UCLASS()
class MYGAME_API AMyAIController : public AAIController
{
	GENERATED_BODY()
public:
	AMyAIController(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My AI Controller")
	float AttackRange = 140.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My AI Controller")
	float ChaseRange = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My AI Controller")
	float AggressiveRange = 300.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "My AI Controller")
	TObjectPtr<class UAISenseConfig_Sight> SightConfig;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);
};
