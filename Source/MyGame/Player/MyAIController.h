// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameplayTagContainer.h"
#include "AssAbilityTemplate.h"
#include "MyAIController.generated.h"

// "When my pawn gets this gameplay event, use these abilities" (its grants of these templates, in order)
USTRUCT(BlueprintType)
struct FAIEventResponse
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Event;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSoftObjectPtr<UAssAbilityTemplate>> Templates;
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
	// AttackRange while the pawn has status.buffedrange, and once it goes (the boss's self debuff; Ass Step4.md
	// section 4), copied into the blackboard key AttackRangeKey too
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "My AI Controller")
	float BuffedAttackRange = 900.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "My AI Controller")
	float UnbuffedAttackRange = 220.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "My AI Controller")
	FName AttackRangeKey = TEXT("AttackRange");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "My AI Controller")
	TObjectPtr<class UAISenseConfig_Sight> SightConfig;

	// Decisions this AI makes on its pawn's gameplay events, e.g. the boss's phase moves on status.health.75 / 50 / 25.
	// The abilities themselves don't trigger on these: the AI chooses to use them.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "My AI Controller")
	TArray<FAIEventResponse> EventResponses;

	/**
	 * Ass Step 7: the enemy runs Pattern on the plugin's driver instead of the behavior tree, stepped in spawn order by
	 * the world's AMyPatternStepper. Its health marks then settle the pattern's phases instead of Event responses.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My AI Controller|Pattern")
	bool bUsePattern = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My AI Controller|Pattern", meta = (EditCondition = "bUsePattern"))
	TObjectPtr<class UMyPatternAsset> Pattern;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "My AI Controller|Pattern")
	TObjectPtr<class UMyPatternDriver> PatternDriver;

	// The pattern's moves, set as the tree moved: a chase that presses on, never pinned, partial paths walked
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "My AI Controller|Pattern")
	TObjectPtr<class UAssMoverComponent> PatternMover;

	UFUNCTION(BlueprintPure, Category = "My AI Controller|Pattern")
	bool UsesPattern() const { return bUsePattern && Pattern != nullptr; }
	/** The pattern's target (the tree's TargetChar), or none. */
	UFUNCTION(BlueprintPure, Category = "My AI Controller|Pattern")
	class ACharacter* GetPatternTarget() const;
	UFUNCTION(BlueprintCallable, Category = "My AI Controller|Pattern")
	void SetPatternTarget(AActor* Target);
	/** One step of the pattern, from the world's stepper. */
	void StepPattern();

	/**
	 * Freezes the pattern or wakes it, as the tests stop and restart a tree's brain (a frozen dummy, an enemy woken):
	 * frozen, it steps no more and stops moving, but keeps its state and its target, and its health marks still settle
	 * (the tree's event responses were the controller's); woken, it steps on (started first, if it never was).
	 */
	UFUNCTION(BlueprintCallable, Category = "My AI Controller|Pattern")
	void SetPatternRunning(bool bRunning);

	/** The frame the pattern last woke (started, or unfrozen): the recorder times the tree's first tasks from it. */
	uint64 GetPatternWokeFrame() const { return PatternWokeFrame; }

	UFUNCTION(BlueprintCallable, Category = "My AI Controller|Pattern")
	bool IsPatternRunning() const;

	/**
	 * Puts this enemy on Asset's pattern now (none: back on the tree), as Use pattern would have at possession: the tree
	 * stops, the pattern registers with the stepper and starts, keeping the tree's target. For tests, before the switch
	 * is on for every enemy.
	 */
	UFUNCTION(BlueprintCallable, Category = "My AI Controller|Pattern")
	void UsePatternNow(class UMyPatternAsset* Asset);

	// The tree doesn't start for an enemy on a pattern (the controllers' Blueprints start it on BeginPlay)
	bool RunBehaviorTree(class UBehaviorTree* BTAsset) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	UFUNCTION()
	void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);
	void OnPawnGameplayEvent(FGameplayTag Event, const struct FGameplayEventData* Payload);
	void OnBuffedRangeChanged(const FGameplayTag Tag, int32 Count);
	void SetAttackRangeEverywhere(float Range);

	TWeakObjectPtr<class UAbilitySystemComponent> PawnAbilitySystem;
	FGameplayTagContainer RespondedEvents;
	FDelegateHandle PawnEventHandle;
	FDelegateHandle BuffedRangeHandle;
	/** Starts the pattern once the pawn has begun play (its grants are given then), if it hasn't started. */
	void EnsurePatternStarted();
	/** The pattern's salt, given by the stepper at possession. */
	int32 PatternSalt = 0;
	/** Frozen by the tests (SetPatternRunning), and when it last woke. */
	bool bPatternFrozen = false;
	uint64 PatternWokeFrame = 0;
};
