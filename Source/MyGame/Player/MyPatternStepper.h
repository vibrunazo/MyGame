#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyPatternStepper.generated.h"

class AMyAIController;

/**
 * Steps every enemy's pattern once a frame, in spawn order (Ass Step 7, Q7; Plan.md 4.3: tokens and ties go in spawn
 * order, never actor tick order). One per world, made by the first enemy that registers; it ticks before the physics, and
 * each registered controller after it, so a pattern decides where the tree did, before its pawn moves.
 */
UCLASS(NotPlaceable, Transient)
class MYGAME_API AMyPatternStepper : public AActor
{
	GENERATED_BODY()

public:
	AMyPatternStepper();

	void Tick(float DeltaSeconds) override;

	/**
	 * Puts a controller in the world's order (the order enemies are possessed: spawn order) and makes it tick after the
	 * stepper. Returns its salt for the pattern's draws: its place among the enemies registered in its tile, mixed with
	 * the run's seed (the value, never the run's stream) and the tile.
	 */
	static int32 Register(AMyAIController* Controller);

	/**
	 * As a new world started by a run with RunSeed (a run or a floor opens its map): the places start again in every
	 * tile and the salts mix RunSeed. For tests, which spawn many enemies in one world: a seeded scenario started from
	 * here salts its enemies as a fresh room of that run would, so it repeats, and another seed draws differently.
	 */
	static void ResetPlaces(UWorld* World, int32 RunSeed);

	/**
	 * A pattern's target moves before the stepper ticks (its ticks in the stepper's group or earlier, its movement's and its
	 * combat component's among them, are the stepper's prerequisites), so every pattern decides, and its controller aims,
	 * from where the target is this frame. Left unordered, which ticked first fell to
	 * the world's tick history: an enemy facing a moving player could turn a frame apart from one world to the next.
	 */
	static void FollowTarget(UWorld* World, const class ACharacter* Target);

private:
	/** The world's stepper, made on first use with the run's seed from the game instance. */
	static AMyPatternStepper* FindOrMake(UWorld* World);

	TArray<TWeakObjectPtr<AMyAIController>> Controllers;
	/** How many enemies registered in each tile, for their places. */
	TMap<FIntPoint, int32> InTile;
	/** The run's seed, read from the game instance when the stepper is made (or given by ResetPlaces). */
	int32 RunSeed = 0;
};
