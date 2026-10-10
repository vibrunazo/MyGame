// The run's room layout, decided without a world: which room data asset goes on which grid cell.
// ALevelBuilder streams the rooms and builds walls and doors from it.

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "LevelLayout.generated.h"

class URoomDataAsset;

USTRUCT(BlueprintType)
struct FLevelLayoutParams
{
	GENERATED_BODY()

	// Rooms on the walk, start and boss included (the treasure room comes on top)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumRooms = 4;

	// The walk goes right (+Y) with this chance (percent); going right lowers it, going up or down raises it
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InitialChanceOfGoingRight = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DecWhenChoseRight = 30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IncWhenChoseVert = 60;
};

USTRUCT(BlueprintType)
struct FPlannedRoom
{
	GENERATED_BODY()

	// X = up / down (Top is +X), Y = right
	UPROPERTY(BlueprintReadOnly)
	FIntPoint Coord = FIntPoint::ZeroValue;

	// Null when the catalogue had no room for that slot
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<URoomDataAsset> Room = nullptr;

	// The doored room every five rooms: clearing it gives the miniboss reward
	UPROPERTY(BlueprintReadOnly)
	bool bMinibossReward = false;
};

USTRUCT(BlueprintType)
struct FLevelLayout
{
	GENERATED_BODY()

	// In walk order, the treasure room last when there is one
	UPROPERTY(BlueprintReadOnly)
	TArray<FPlannedRoom> Rooms;

	const FPlannedRoom* Find(FIntPoint Coord) const { return Rooms.FindByPredicate([Coord](const FPlannedRoom& R) { return R.Coord == Coord; }); }
};

struct MYGAME_API FLevelLayoutGenerator
{
	// Same params, catalogue (in the same order) and stream state give the same layout
	static FLevelLayout Generate(const FLevelLayoutParams& Params, const TArray<URoomDataAsset*>& Catalogue, FRandomStream& Stream);
};
