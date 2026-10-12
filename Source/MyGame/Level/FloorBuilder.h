// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Containers/Ticker.h"
#include "TileTemplate.h"
#include "FloorLayout.h"
#include "../Abilities/LootComponent.h"
#include "FloorBuilder.generated.h"

UENUM(BlueprintType)
enum class ETileSide : uint8
{
	Left,
	Right,
	Bottom,
	Top
};

USTRUCT(BlueprintType)
struct FWallSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Length = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDoored = false;
};

USTRUCT(BlueprintType)
struct FTileState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTileTemplate* Template = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class ULevelStreaming* LevelRef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class ATileMaster* TileMasterRef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<ETileSide> Walls = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsCleared = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<FLootDrop> Reward;
};

// A room (CLAUDE.md glossary): tiles joined with no wall between them. The camera and the darkness work per room
USTRUCT()
struct FFloorRoom
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FIntPoint> Tiles;
};

USTRUCT()
struct FTileCoord
{
	GENERATED_BODY()
	// FString ID;
	int32 ID = 0;
	int16 X = 0;
	int16 Y = 0;
	// bool operator== (const FTileCoord& Other)
	// {
	// 	// return ID == Other.ID;
	// 	return false;
	// }
	friend bool operator==(const FTileCoord& first, const FTileCoord& second)
	{
		return (first.ID == second.ID);
	}
	friend uint32 GetTypeHash(const FTileCoord& Other)
	{
		return GetTypeHash(Other.ID);
	}
	explicit FTileCoord(int16 NewX, int16 NewY)
        : ID (NewX * 100000 + NewY)
        , X(NewX)
        , Y(NewY)
    {
    }
	explicit FTileCoord()
        : ID(0)
        , X(0)
        , Y(0)
    {
    }
	FString ToString() { return FString::Printf(TEXT("%d,%d"), X, Y);}
};

//DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FEnterTileSignature, AFloorBuilder, OnEnterTile, FTileState, NewRoom);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnterTileSignature, FTileState, NewRoom);

UCLASS()
class MYGAME_API AFloorBuilder : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFloorBuilder();
	void OnConstruction(const FTransform & Transform) override;

	// Links a room master to the room whose streamed map it's in; returns that room, or null
	FTileState* RegisterTileMaster(ATileMaster* NewTileMaster);
	// The room streamed in as this level, or null
	FTileState* GetTileOfLevel(const ULevel* Level);
	void OnEnterTile(FTileState NewRoom);
	class UTileTemplate* GetTemplateAt(FTileCoord Coord);
	FTileState* GetTileAt(FTileCoord Coord);
	class AWall* GetBottomWallFromLoc(FVector Location);
	class AWall* GetWallAtTileSide(FTileCoord Coord, ETileSide Dir);
	void OnUpdateCharCoord(FVector Location, ETileSide Dir=ETileSide::Bottom);
	void HideWall(FTileCoord Coord, ETileSide Dir=ETileSide::Bottom);
	void HideOneWall(AWall* ThisWall);
	void SetTileCleared(FTileState& Room);
	void OpenDoors();
	void CloseDoors();
	void TeleportPlayerInsideTile(FVector NewLocation);
	void AggroTile(class APawn* PlayerToAggro, FVector Location);
	FVector GetLocFromTile(FTileCoord Coord);
	FTileCoord GetTileFromLoc(FVector Location);
	// The room a tile belongs to, -1 for a cell with no tile
	UFUNCTION(BlueprintCallable, Category = "FloorBuilder")
	int32 GetRoomOfTile(FIntPoint Tile) const;
	const TArray<FIntPoint>& GetRoomTiles(int32 Room) const;
	// Two neighboring tiles with no wall between them
	bool AreJoined(const FIntPoint& A, const FIntPoint& B) const;
	int32 NumRooms() const { return Rooms.Num(); }
	// The smallest and largest tile coordinates on this floor
	void GetTileExtent(FIntPoint& Min, FIntPoint& Max) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FloorBuilder")
	FVector GetTileCenter(FVector CurrentLocation);


	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = FloorBuilder)
	class UBillboardComponent* BBComp;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = FloorBuilder)
	class UBoxComponent* TileBounds;
	// Every room data asset, sorted by name so a seed gives the same level on every machine
	UPROPERTY()
	TArray<TObjectPtr<class UTileTemplate>> TileTemplates;
	UPROPERTY()
	TArray<TObjectPtr<class ADoor>> DoorList;
	UPROPERTY()
	TArray<TObjectPtr<class ATileDressing>> Dressings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloorBuilder)
	int32 NumTiles = 7;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloorBuilder)
	int32 InitialChanceOfGoingRight = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloorBuilder)
	int32 DecWhenChoseRight = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloorBuilder)
	int32 IncWhenChoseVert = 35;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloorBuilder)
	TSubclassOf<class AWall> WallActor = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloorBuilder)
	TSubclassOf<class ADoor> DoorActor;
	// Scatters props along each room's solid walls (PCG_RoomDressing); none when unset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorBuilder|Dressing")
	TObjectPtr<class UPCGGraphInterface> DressingGraph;
	// How far from a wall the props' line runs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorBuilder|Dressing")
	float DressingInset = 110.f;
	// Half the prop-free gap kept around a doorway
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorBuilder|Dressing")
	float DoorwayClearance = 260.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloorBuilder)
	float TileSizeX = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloorBuilder)
	float TileSizeY = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorBuilder")
	class USoundBase* LevelMusic;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloorBuilder)
	TArray<FLootDrop> MinibossReward;
	UPROPERTY()
	class UAudioComponent* LevelMusicRef = nullptr;
	UPROPERTY()
	class UAudioComponent* BossMusicRef = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorBuilder")
	class USoundBase* BossMusic;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloorBuilder)
	// int32 RandomSeed = 0;
	FRandomStream* RandomStream = nullptr;
	// This run's plan, and the Tiles built from it (room states by coordinate)
	UPROPERTY()
	FFloorLayout Layout;
	UPROPERTY()
	TMap<FTileCoord, FTileState> Tiles;
	UPROPERTY()
	TMap<FString, class AWall*> AllWalls;

	UPROPERTY(BlueprintAssignable, Category = "FloorBuilder")
	FEnterTileSignature OnEnterTileDelegate;
	

private:
	void SpawnLevels();
	void PlanTiles();
	// Groups the tiles into rooms once the walls are known
	void FindRooms();
	UPROPERTY()
	TArray<FFloorRoom> Rooms;
	TMap<FIntPoint, int32> RoomOfTile;
	FString DebugTiles();
	void BuildWalls(TPair<FTileCoord, FTileState> &Tile);
	class ULevelStreaming* SpawnTile(FTileCoord Where, class UTileTemplate* Template);
	void LoadTileTemplates();
	class ADoor* SpawnDoor(FTileCoord Where, ETileSide Dir);
	class AWall* SpawnWall(FTransform Where, FWallSettings* Settings = nullptr);
	class AWall* SpawnWallAtLocDirSettings(FTransform Where, ETileSide Pos, FWallSettings* Settings = nullptr);
	class AWall* TrySpawnWallCoordDir(TPair<FTileCoord, FTileState> &Tile, ETileSide Pos, bool Doored);
	class AWall* TrySpawnWallFromSettings(TPair<FTileCoord, FTileState>& Tile, ETileSide Pos, FWallSettings* Settings = nullptr);
	class AWall* TrySpawnEdgeWallAtCoord(TPair<FTileCoord, FTileState> &Tile, ETileSide Pos);
	FTransform GetWallLocFromTileAndSide(FTileCoord Coord, ETileSide Dir);
	FString GetWallID(FTileCoord Coord1, FTileCoord Coord2);
	FString GetWallID(FTileCoord Coord, ETileSide Dir);
	FTileCoord GetNeighbor(FTileCoord From, ETileSide To);
	bool IsTileOfType(FTileCoord Tile, ETileType Type);
	ETileSide GetOppositeDirection(ETileSide From);
	// class UStaticMesh* GetWallTypeAtTiles(FTileCoord Coord1, FTileCoord Coord2, bool Cap = false);
	UFUNCTION()
	void OnLoadedOneLevel();
	void OnLoadingTimedOut();
	// Every room is in (or loading gave up): unfreeze and dress the rooms
	void OnTilesReady();
	bool bTilesReady = false;
	void DressTile(const FTileCoord& Coord, const FTileState& Room);
	// A core ticker, not a world timer: world timers run on dilated time, and the world is frozen while rooms stream
	FTSTicker::FDelegateHandle LoadingTimeout;

	UPROPERTY()
	TArray<class AWall*> HiddenWalls;
	FTileCoord LastEnteredTile = FTileCoord(-67, 9390);
	uint8 CountOfLevelsThatDidntFinishLoading = 0;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


};
