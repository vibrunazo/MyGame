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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TArray<FEffectContainer> EffectsToApply;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item")
	FGameplayTag CueTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (tooltip = "If it's not a consumable, it gets added to the GameInstance Inventory. So it gets applied on BeginPlay when respawning/new levels."))
	bool bIsConsumable = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	uint8 MaxStacks = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item")
	TSubclassOf<class APickup> PickupActor;

	// The Class of the Actor for the visible Mesh of this Item when dropped in the world
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSubclassOf<class APickupMeshActor> PickupMeshActorClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class UStaticMesh* PickupMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class UNiagaraSystem* SpawnParticles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class USoundBase* SpawnSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class USoundBase* PickupSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class UNiagaraSystem* PickupParticles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	bool bMaxHPCanPickup = true;
};
