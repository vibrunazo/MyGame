// Fill out your copyright notice in the Description page of Project Settings.


#include "LootComponent.h"
#include "../MyGameInstance.h"
#include "../Props/ItemDataAsset.h"
#include "../Props/Pickup.h"

#include "Kismet/GameplayStatics.h"

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
	UMyGameInstance* GI = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(this));
	if (LootTable.Num() > 0 && GI)
	{
		TArray<FString> ItemsToFilter = GI->GetItemsICannotGetMoreOf();
		TArray<FLootDrop> FilteredLootTable = TArray<FLootDrop>();
		for (auto&& Item : LootTable)
		{
			if (!ItemsToFilter.Contains(Item.Item->ItemName.ToString()))
			{
				FilteredLootTable.Add(Item);
			}
		}
		if (FilteredLootTable.Num() > 0)
		{
			int RandIndex = GI->RandomStream.RandRange(0, FilteredLootTable.Num() - 1);
			result = FilteredLootTable[RandIndex].Item;
		}
		else
		{
			int RandIndex = GI->RandomStream.RandRange(0, LootTable.Num() - 1);
			result = LootTable[RandIndex].Item;
		}
	}
	return result;
}

APickup* ULootComponent::DropRandomItemAtLocation(FVector Where)
{
		// UE_LOG(LogTemp, Warning, TEXT("dropping items"));
	FVector Loc = Where;
	FActorSpawnParameters params;
	auto NewLoot = GetRandomItem();
	if (!NewLoot) return nullptr;
	APickup* NewPickup = GetWorld()->SpawnActor<APickup>(NewLoot->PickupActor, Loc, FRotator::ZeroRotator, params);
	NewPickup->SetItemData(NewLoot);
	UE_LOG(LogTemp, Warning, TEXT("Lootcomponent dropped a %s"), *NewLoot->GetName());
	return nullptr;
}
