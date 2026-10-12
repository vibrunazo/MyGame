// A room's props: lines along its solid walls, and a PCG component that scatters props on them
// (PCG_RoomDressing: sample, seeded thinning, pruning, down onto the floor, weighted meshes).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileDressing.generated.h"

class UPCGComponent;
class UPCGGraphInterface;
class USplineComponent;

UCLASS()
class MYGAME_API ATileDressing : public AActor
{
	GENERATED_BODY()

public:
	ATileDressing();

	// A straight line, in world space, that props may line up along
	void AddLine(const FVector& From, const FVector& To);

	// The room's half size: PCG works within the actor's bounds
	void SetTileExtent(const FVector& HalfSize);

	// Scatters the props; the same seed gives the same props
	void Generate(UPCGGraphInterface* Graph, int32 Seed);

	int32 NumLines() const { return Lines.Num(); }

	UPROPERTY(VisibleAnywhere, Category = Dressing)
	TObjectPtr<class UBoxComponent> Bounds;
	UPROPERTY(VisibleAnywhere, Category = Dressing)
	TObjectPtr<UPCGComponent> PCG;

private:
	UPROPERTY()
	TArray<TObjectPtr<USplineComponent>> Lines;
};
