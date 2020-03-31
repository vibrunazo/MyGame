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
	int16 X = 0;
	int16 Y = 0;

	FString ID;
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
        : ID (FGuid::NewGuid().ToString())
        , X(NewX)
        , Y(NewY)
    {
    }
	explicit FCoord()
        : ID (FGuid::NewGuid().ToString())
        , X(0)
        , Y(0)
    {
    }
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LevelBuilder)
	class UBillboardComponent* BBComp;
	TArray<FAssetData> AssetDataList;
	TArray<class URoomDataAsset*> RoomList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyLibrary)
	class UStaticMesh* WallMesh;
	TMap<FCoord, FGridStruct> Grid;
	

private:
	void GenerateLevels();
	void BuildGrid();
	class ULevelStreaming* GenerateRoom(FTransform Where);
	void SetAssetListFromRegistry();
	class URoomDataAsset* GetRandomRoom();
	class AStaticMeshActor* GenerateWall(FTransform Where);
	class AStaticMeshActor* GenerateWall(FTransform Where, EWallPos Pos);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


};
