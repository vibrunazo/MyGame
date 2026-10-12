// The run's room layout, decided without a world: which room data asset goes on which grid cell.
// AFloorBuilder streams the rooms and builds walls and doors from it.

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "FloorLayout.generated.h"

class UTileTemplate;

USTRUCT(BlueprintType)
struct FFloorLayoutParams
{
	GENERATED_BODY()

	// Tiles on the walk, start and boss included (the treasure room comes on top)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumTiles = 4;

	// The walk goes right (+Y) with this chance (percent); going right lowers it, going up or down raises it
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InitialChanceOfGoingRight = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DecWhenChoseRight = 30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IncWhenChoseVert = 60;
};

USTRUCT(BlueprintType)
struct FPlannedTile
{
	GENERATED_BODY()

	// X = up / down (Top is +X), Y = right
	UPROPERTY(BlueprintReadOnly)
	FIntPoint Coord = FIntPoint::ZeroValue;

	// Null when the catalogue had no room for that slot
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTileTemplate> Template = nullptr;

	// The doored room every five rooms: clearing it gives the miniboss reward
	UPROPERTY(BlueprintReadOnly)
	bool bMinibossReward = false;
};

USTRUCT(BlueprintType)
struct FFloorLayout
{
	GENERATED_BODY()

	// In walk order, the treasure room last when there is one
	UPROPERTY(BlueprintReadOnly)
	TArray<FPlannedTile> Tiles;

	const FPlannedTile* Find(FIntPoint Coord) const { return Tiles.FindByPredicate([Coord](const FPlannedTile& R) { return R.Coord == Coord; }); }
};

struct MYGAME_API FFloorLayoutGenerator
{
	// Same params, catalogue (in the same order) and stream state give the same layout
	static FFloorLayout Generate(const FFloorLayoutParams& Params, const TArray<UTileTemplate*>& Catalogue, FRandomStream& Stream);
};
