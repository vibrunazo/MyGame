// Fill out your copyright notice in the Description page of Project Settings.


#include "LootComponent.h"
#include "../MyGameInstance.h"
#include "../Props/ItemDataAsset.h"
#include "../Props/LearnItemDataAsset.h"
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


/// <summary>
/// Compares 2 list of abilities and checks if any ability from one list shares a common Slot (Input button) with any ability of the second list.
/// Used to decide if an Item that teaches an ability should drop. If an item teaches an ability for a slot the player already has, it should not drop.
/// LootComponent uses this to compare the list of abilities an Item can teach to the list of abilities the player already know.
/// </summary>
/// <param name="FirstList">The first list of abilities to compare</param>
/// <param name="SecondList">The second list of abilities to compare</param>
/// <returns>True if any ability of the first list has the same slot as any ability of the second list</returns>
bool ULootComponent::DoAnyAbilitySlotOverlap(TArray<struct FAbilityStruct> FirstList, TArray<struct FAbilityStruct> SecondList)
{
	for (auto&& FirstAbility : FirstList)
	{
		for (auto&& SecondAbility : SecondList)
		{
			if (FirstAbility.Input == SecondAbility.Input) return true;
		}
	}
	return false;
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
	if (LootTable.Num() == 0 || !GI) return result;
	TArray<FLootDrop> FilteredLootTable = TArray<FLootDrop>();
	// gets items I am already maxed on
	TArray<FString> ItemsToFilter = GI->GetItemsICannotGetMoreOf();
	// get abilities I already have a slot for
	auto MyAbilities = GI->GetPlayerAbiliies();
	// loop through all Items that I could potentially drop
	for (auto&& Item : LootTable)
	{
		// filter out the items I'm maxed on
		bool IsMaxed = ItemsToFilter.Contains(Item.Item->ItemName.ToString());
		// now check if I should also filter by abillities slot to learn
		ULearnItemDataAsset* LearnItem = Cast<ULearnItemDataAsset>(Item.Item);
		bool IsAbilitySlotUsed = false;
		if (LearnItem)
		{
			// filter out learn items that teach abilties for slots I already know an ability 
			if (DoAnyAbilitySlotOverlap(LearnItem->AbilitiesToLearn, MyAbilities)) IsAbilitySlotUsed = true;
		}
		// If this Item passes the tests, add it to filtered list
		if (!IsMaxed && !IsAbilitySlotUsed)
		{
			FilteredLootTable.Add(Item);
		}
	}

	// if the filtered list is not zero, then return an item from the filtered list
	if (FilteredLootTable.Num() > 0)
	{
		int RandIndex = GI->RandomStream.RandRange(0, FilteredLootTable.Num() - 1);
		result = FilteredLootTable[RandIndex].Item;
	}
	// else, if the filtered list is empty, return an item I already have anyway
	else
	{
		int RandIndex = GI->RandomStream.RandRange(0, LootTable.Num() - 1);
		result = LootTable[RandIndex].Item;
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
