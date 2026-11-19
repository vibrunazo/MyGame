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

private:
	TArray<TWeakObjectPtr<AMyAIController>> Controllers;
	/** How many enemies registered in each tile, for their places. */
	TMap<FIntPoint, int32> InTile;
};
