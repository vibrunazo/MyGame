// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
struct FGridStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class URoomDataAsset* RoomType = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class ULevelStreaming* RoomRef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EWallPos> Walls = {};
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
	class AStaticMeshActor* GetBottomWallFromLoc(FVector Location);
	class AStaticMeshActor* GetWallRefFromCoordAndDir(FCoord Coord, EWallPos Dir);
	void HideWall(FVector Location, EWallPos Dir=EWallPos::Bottom);
	void HideWall(FCoord Coord, EWallPos Dir=EWallPos::Bottom);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LevelBuilder)
	class UBillboardComponent* BBComp;
	TArray<FAssetData> AssetDataList;
	TArray<class URoomDataAsset*> RoomList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyLibrary)
	class UStaticMesh* WallMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyLibrary)
	class UStaticMesh* WallDooredMesh;
	TMap<FCoord, FGridStruct> Grid;
	TMap<FString, class AStaticMeshActor*> AllWalls;
	

private:
	void GenerateLevels();
	void BuildGrid();
	void BuildWalls(TPair<FCoord, FGridStruct> Tile);
	class ULevelStreaming* GenerateRandomRoom(FTransform Where);
	class ULevelStreaming* GenerateRoom(FCoord Where, class URoomDataAsset* RoomType);
	void SetAssetListFromRegistry();
	class URoomDataAsset* GetRandomRoom();
	class AStaticMeshActor* GenerateWall(FTransform Where, UStaticMesh* What = nullptr);
	class AStaticMeshActor* GenerateWallAtLoc(FTransform Where, EWallPos Pos, UStaticMesh* What = nullptr);
	class AStaticMeshActor* GenerateWallAtGrid(FCoord Where, EWallPos Pos, UStaticMesh* What = nullptr);
	class AStaticMeshActor* GenerateEdgeWallAtGrid(FCoord Where, EWallPos Pos);
	FVector GetLocFromGrid(FCoord Coord);
	FCoord GetGridFromLoc(FVector Location);
	FString GetWallID(FCoord Coord1, FCoord Coord2);
	FString GetWallID(FCoord Coord, EWallPos Dir);
	FCoord GetNeighbor(FCoord From, EWallPos To);

	TArray<class AStaticMeshActor*> HiddenWalls;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


};
