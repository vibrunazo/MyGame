// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LootComponent.generated.h"


USTRUCT(BlueprintType)
struct FLootDrop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UItemDataAsset* Item = nullptr;

	/** Relative weight against the other candidates. Items at 0 only drop when every candidate is at 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 DropRate = 0;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYGAME_API ULootComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULootComponent();

	UFUNCTION(BlueprintCallable, Category = Loot)
	class UItemDataAsset* GetRandomItem();
	class APickup* DropRandomItemAtLocation(FVector Where);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<FLootDrop> LootTable;

private:
	bool DoAnyAbilitySlotOverlap(TArray<struct FAbilityStruct> FirstList, TArray<struct FAbilityStruct> SecondList);
	uint8 DoAnyAbilityOverlap(TArray<struct FAbilityStruct> FirstList, TArray<struct FAbilityStruct> SecondList);
	static class UItemDataAsset* PickWeighted(const TArray<FLootDrop>& Candidates, FRandomStream& Stream);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
