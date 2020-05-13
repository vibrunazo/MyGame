// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoomDataAsset.generated.h"


UENUM(BlueprintType)
enum class ERoomType : uint8
{
	Empty 		UMETA(DisplayName="Empty"),
	Enemies 	UMETA(DisplayName="Enemies"),
	Boss		UMETA(DisplayName="Boss"),
	Treasure
};

/**
 * 
 */
UCLASS()
class MYGAME_API URoomDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FName GetAutoLevelAddress();

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FName LevelAddress = "Game/Maps/Rooms/Room01";
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	ERoomType RoomType = ERoomType::Empty;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	int32 RoomDifficulty = 0;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool bIsWalled = false;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool bIsDoored = false;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FVector RoomSize = FVector(2000.f, 2000.f, 1000.f);
	
};
