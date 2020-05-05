// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
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
	FText ItemName = FText::FromString(TEXT("Item"));
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item")
	FText Description = FText();
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item")
	TSubclassOf<class APickup> PickupActor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TArray<FEffectContainer> EffectsToApply;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item")
	FGameplayTag CueTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bIsConsumable = false;
};
