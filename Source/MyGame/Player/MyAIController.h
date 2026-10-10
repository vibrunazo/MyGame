// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameplayTagContainer.h"
#include "MyAIController.generated.h"

// "When my pawn gets this gameplay event, use these abilities" (in order)
USTRUCT(BlueprintType)
struct FAIEventResponse
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Event;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;
};

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

	// Decisions this AI makes on its pawn's gameplay events, e.g. the boss's phase moves on status.health.75 / 50 / 25.
	// The abilities themselves don't trigger on these: the AI chooses to use them.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "My AI Controller")
	TArray<FAIEventResponse> EventResponses;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	UFUNCTION()
	void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);
	void OnPawnGameplayEvent(FGameplayTag Event, const struct FGameplayEventData* Payload);

	TWeakObjectPtr<class UAbilitySystemComponent> PawnAbilitySystem;
	FGameplayTagContainer RespondedEvents;
	FDelegateHandle PawnEventHandle;
};
