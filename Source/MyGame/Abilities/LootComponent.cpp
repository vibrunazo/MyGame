// Fill out your copyright notice in the Description page of Project Settings.


#include "LootComponent.h"
#include "../MyGameInstance.h"
#include "../Props/ItemDataAsset.h"
#include "../Props/LearnItemDataAsset.h"
#include "../Props/Pickup.h"

#include "Kismet/GameplayStatics.h"
#include "Abilities/GameplayAbility.h"

// Sets default values for this component's properties
ULootComponent::ULootComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


/// <summary>
/// Compares 2 lists of abilities and checks if any ability from one list shares a common Slot (Input button) with any ability of the second list.
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

/// <summary>
/// Compares 2 lists of abilities and check if any ability from one list overlap any ability from the second list.
/// Will check both if the abilities share a common slot (input button) and also if the abilities are identical.
/// LootComponent uses this to compare the list of abilities an Item can teach to the list of abilities the player already knows. 
/// </summary>
/// <param name="FirstList">The first list of abilities to compare</param>
/// <param name="SecondList">The second list of abilities to compare</param>
/// <returns>0 if no overlap is found. 1 if any ability from one list shares the same Slot as any ability from the second. 2 if any ability from one list is identical to any ability in the other list.</returns>
uint8 ULootComponent::DoAnyAbilityOverlap(TArray<struct FAbilityStruct> FirstList, TArray<struct FAbilityStruct> SecondList)
{
	uint8 result = 0;
	for (auto&& FirstAbility : FirstList)
	{
		for (auto&& SecondAbility : SecondList)
		{
			if (FirstAbility.AbilityClass == SecondAbility.AbilityClass) return 2;
			if (FirstAbility.Input == SecondAbility.Input) result = 1;
		}
	}
	return result;
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
	TArray<FLootDrop> FilteredLootTableNewSpell = TArray<FLootDrop>();
	TArray<FLootDrop> FilteredLootTableNotMaxed = TArray<FLootDrop>();
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
		bool IsAbilityAlreadyLearned = false;
		if (LearnItem)
		{
			uint8 CheckAbilityOverlap = DoAnyAbilityOverlap(LearnItem->AbilitiesToLearn, MyAbilities);
			// filter out learn items that teach abilties for slots I already know an ability 
			if (CheckAbilityOverlap >= 1) IsAbilitySlotUsed = true;
			// filter out learn items that teach an exact ability that I already know
			if (CheckAbilityOverlap == 2) IsAbilityAlreadyLearned = true;
		}
		// If this Item is not maxed, at it to the filtered list of not maxed items
		if (!IsMaxed)
		{
			FilteredLootTableNotMaxed.Add(Item);
			// If this Item passes all tests, add it to full filtered list
			if (!IsAbilitySlotUsed)
			{
				FilteredLootTable.Add(Item);
			}
		}
		// If teaches a new ability, even if maxed
		if (!IsAbilityAlreadyLearned)
		{
			FilteredLootTableNewSpell.Add(Item);
		}
	}
	// if the filtered list is not zero, that is, if there are items that teach new spells on new slots, then return an item from the filtered list
	if (FilteredLootTable.Num() > 0)
	{
		int RandIndex = GI->RandomStream.RandRange(0, FilteredLootTable.Num() - 1);
		result = FilteredLootTable[RandIndex].Item;
	}
	// else, if the full filtered loot table is zero, but there are still items that teach abilities I don't have, drop one of those even if it doesn't pass all tests, at least gets a new ability
	else if (FilteredLootTableNewSpell.Num() > 0)
	{
		int RandIndex = GI->RandomStream.RandRange(0, FilteredLootTableNewSpell.Num() - 1);
		result = FilteredLootTableNewSpell[RandIndex].Item;
	}
	// else, if neither the full filtered loot table or the new spell filter have anything, but there are still items that I don't have, drop one of those even if it doesn't pass all tests, at least gets a new item
	else if (FilteredLootTableNotMaxed.Num() > 0)
	{
		int RandIndex = GI->RandomStream.RandRange(0, FilteredLootTableNotMaxed.Num() - 1);
		result = FilteredLootTableNotMaxed[RandIndex].Item;
	}
	// else, if all filtered lists are empty, return an item I already have anyway
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
