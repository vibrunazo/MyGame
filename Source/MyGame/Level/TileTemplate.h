// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TileTemplate.generated.h"


UENUM(BlueprintType)
enum class ETileType : uint8
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
class MYGAME_API UTileTemplate : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// The room's map, streamed in as a level instance at the room's grid cell
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSoftObjectPtr<UWorld> Level;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	ETileType TileType = ETileType::Empty;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	int32 Difficulty = 0;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool bIsWalled = false;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool bIsDoored = false;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FVector RoomSize = FVector(2000.f, 2000.f, 1000.f);
	
};
