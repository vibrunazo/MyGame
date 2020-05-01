// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
// #include "GameplayEffectTypes.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "ItemDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item")
	FString ItemName = "Item Name";
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item")
	TSubclassOf<class APickup> PickupActor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TArray<FEffectContainer> EffectsToApply;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bIsConsumable = false;
};
