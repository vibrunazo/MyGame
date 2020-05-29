// Fill out your copyright notice in the Description page of Project Settings.


#include "LootComponent.h"

// Sets default values for this component's properties
ULootComponent::ULootComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void ULootComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	// UE_LOG(LogTemp, Warning, TEXT("Loot Component INIT"));
	
}


// Called every frame
void ULootComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

UItemDataAsset* ULootComponent::GetRandomItem()
{
	UItemDataAsset* result = nullptr;
	if (LootTable.Num() > 0)
	{
		int RandIndex = FMath::RandRange(0, LootTable.Num() - 1);
		result = LootTable[RandIndex].Item;
	}
	return result;
}