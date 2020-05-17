// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomDataAsset.h"
#include "LevelBuilder.generated.h"

UENUM(BlueprintType)
enum class EWallPos : uint8
{
	Left,
	Right,
	Bottom,
	Top
};

USTRUCT(BlueprintType)
struct FRoomState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class URoomDataAsset* RoomType = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class ULevelStreaming* RoomRef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EWallPos> Walls = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsRoomCleared = false;
};

USTRUCT()
struct FCoord
{
	GENERATED_BODY()
	// FString ID;
	int32 ID = 0;
	int16 X = 0;
	int16 Y = 0;
	// bool operator== (const FCoord& Other)
	// {
	// 	// return ID == Other.ID;
	// 	return false;
	// }
	friend bool operator==(const FCoord& first, const FCoord& second)
	{
		return (first.ID == second.ID);
	}
	friend uint32 GetTypeHash(const FCoord& Other)
	{
		return GetTypeHash(Other.ID);
	}
	explicit FCoord(int16 NewX, int16 NewY)
        : ID (NewX * 100000 + NewY)
        , X(NewX)
        , Y(NewY)
    {
    }
	explicit FCoord()
        : ID(0)
        , X(0)
        , Y(0)
    {
    }
	FString ToString() { return FString::Printf(TEXT("%d,%d"), X, Y);}
};



UCLASS()
class MYGAME_API ALevelBuilder : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALevelBuilder();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="LevelBuilder")
	class ULevelStreaming* OnBPCreateLevelByName(FName LevelName);
	class URoomDataAsset* GetRoomFromCoord(FCoord Coord);
	FRoomState* GetRoomStateFromCoord(FCoord Coord);
	class AStaticMeshActor* GetBottomWallFromLoc(FVector Location);
	class AStaticMeshActor* GetWallRefFromCoordAndDir(FCoord Coord, EWallPos Dir);
	void OnUpdateCharCoord(FVector Location, EWallPos Dir=EWallPos::Bottom);
	void HideWall(FCoord Coord, EWallPos Dir=EWallPos::Bottom);
	void SetRoomClearedAtLoc(FVector Location);
	void OpenDoors();
	void CloseDoors();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = LevelBuilder)
	class UBillboardComponent* BBComp;
	TArray<FAssetData> AssetDataList;
	TArray<class URoomDataAsset*> RoomList;
	TArray<class ADoor*> DoorList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	int32 NumRooms = 7;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	int32 InitialChanceOfGoingRight = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	int32 DecWhenChoseRight = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	int32 IncWhenChoseVert = 35;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	class UStaticMesh* WallMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	class UStaticMesh* WallDooredMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	class UStaticMesh* WallCappedMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	class UStaticMesh* WallDooredCappedMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	TSubclassOf<class ADoor> DoorActor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	float RoomSizeX = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilder)
	float RoomSizeY = 2000.f;
	TMap<FCoord, FRoomState> Grid;
	TMap<FString, class AStaticMeshActor*> AllWalls;
	

private:
	void GenerateLevels();
	void BuildGrid();
	void BuildWalls(TPair<FCoord, FRoomState> Tile);
	class ULevelStreaming* GenerateRandomRoom(FTransform Where);
	class ULevelStreaming* GenerateRoom(FCoord Where, class URoomDataAsset* RoomType);
	void SetAssetListFromRegistry();
	class URoomDataAsset* GetRandomRoom();
	class URoomDataAsset* GetRandomRoom(int32 Difficulty);
	class URoomDataAsset* AddTreasureRoom();
	class URoomDataAsset* AddTreasureRoomNextTo(FCoord Coord);
	class ADoor* SpawnDoor(FCoord Where, EWallPos Dir);
	class AStaticMeshActor* GenerateWall(FTransform Where, UStaticMesh* What = nullptr);
	class AStaticMeshActor* GenerateWallAtLoc(FTransform Where, EWallPos Pos, UStaticMesh* What = nullptr);
	class AStaticMeshActor* GenerateWallAtGrid(FCoord Where, EWallPos Pos, bool Doored);
	class AStaticMeshActor* GenerateWallMeshAtGrid(FCoord Where, EWallPos Pos, UStaticMesh* What = nullptr);
	class AStaticMeshActor* GenerateEdgeWallAtGrid(FCoord Where, EWallPos Pos);
	TArray<class URoomDataAsset*> FindRoomsOfType(ERoomType Type, int32 Difficulty = -1);
	TArray<class URoomDataAsset*> FindRoomsOfDifficulty(int32 Difficulty);
	FVector GetLocFromGrid(FCoord Coord);
	FTransform GetWallLocFromGridAndDir(FCoord Coord, EWallPos Dir);
	FCoord GetGridFromLoc(FVector Location);
	FString GetWallID(FCoord Coord1, FCoord Coord2);
	FString GetWallID(FCoord Coord, EWallPos Dir);
	FCoord GetNeighbor(FCoord From, EWallPos To);
	bool IsNeighborFree(FCoord From, EWallPos To);
	bool IsAnyNeighborOfType(FCoord From, ERoomType Type);
	TArray<FCoord> GetAllNeighborsCoords(FCoord From);
	TArray<FCoord> FindFreeNeighbors(FCoord From);
	class UStaticMesh* GetWallTypeAtTiles(FCoord Coord1, FCoord Coord2, bool Cap = false);
	UFUNCTION()
	void OnLoadedOneLevel();

	TArray<class AStaticMeshActor*> HiddenWalls;
	FCoord LastEnteredRoomCoord = FCoord(-67, 9390);
	TArray<class AStaticMeshActor*> CappedWalls;
	uint8 CountOfLevelsThatDidntFinishLoading = 0;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


};
