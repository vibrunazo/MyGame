// Fill out your copyright notice in the Description page of Project Settings.


#include "TileMaster.h"
#include "Components/BillboardComponent.h"

#include "../Abilities/LootComponent.h"
#include "../Player/MyCharacter.h"
#include "../Player/ReportDeath.h"
#include "FloorBuilder.h"
#include "RunSubsystem.h"
#include "Goal.h"

// Sets default values
ATileMaster::ATileMaster()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Dino"));
	RootComponent = Billboard;

	LootComponent = CreateDefaultSubobject<ULootComponent>(TEXT("Loot Component"));

}

// Called when the game starts or when spawned
void ATileMaster::BeginPlay()
{
	Super::BeginPlay();

	ULevel* MyLevel = GetLevel();
	UE_LOG(LogTemp, Warning, TEXT("Room Level: %s, persistent: %d"), *MyLevel->GetOuter()->GetName(), IsInPersistentLevel());
	TArray<AActor *> AllActors = MyLevel->Actors;
	for (auto &&Actor : AllActors)
	{
		if (!Actor) continue;
		AMyCharacter* Char = Cast<AMyCharacter>(Actor);
		if (Char)
		{
			AddNewCharToTile(Char);
		}
		AGoal* NewGoal = Cast<AGoal>(Actor);
		if (NewGoal)
		{
			Goals.Add(NewGoal);
		}

	}
	FloorBuilderRef = URunSubsystem::GetFloorBuilder(this);
	if (!FloorBuilderRef)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: no FloorBuilder in this world, room will not be tracked"), *GetName());
		return;
	}
	if (FTileState* Room = FloorBuilderRef->RegisterTileMaster(this)) LootComponent->LootTable = Room->Reward;
	
}

void ATileMaster::AddNewCharToTile(AMyCharacter* Char)
{
	CharsToKill.Add(Char);
	if (!Char)
	{
		UE_LOG(LogTemp, Error, TEXT("Spawner could not bind to Char"));
		return;
	}
	Char->OnDieDelegate.AddUObject(this, &ATileMaster::OnCharDied);
}

// Called every frame
void ATileMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FTileState* ATileMaster::GetTile() const
{
	return FloorBuilderRef ? FloorBuilderRef->GetTileOfLevel(GetLevel()) : nullptr;
}

bool ATileMaster::AreAllCharsDead()
{
	for (auto &&Char : CharsToKill)
	{
		if (!Char || !FloorBuilderRef) continue;
		bool bIsInMyTile = (FloorBuilderRef->GetTileFromLoc(Char->GetActorLocation()) == FloorBuilderRef->GetTileFromLoc(GetActorLocation()));
		if (Char->IsAlive() && bIsInMyTile) return false;
	}
	return true;
}

TArray<class AMyCharacter*> ATileMaster::GetChars()
{
	return CharsToKill;
}

/// <summary>
/// Aggroes all chars that spawned on this room to te given Player
/// </summary>
/// <param name="PlayerRef">Player to Aggro all chars on</param>
void ATileMaster::AggroAll(APawn* PlayerRef)
{
	if (bAggroed) return;
	bAggroed = true;
	for (auto&& Enemy : CharsToKill)
	{
		Enemy->SetAggroTarget(PlayerRef);
	}
}

void ATileMaster::OnCharDied(AMyCharacter* WhoDied)
{
	UE_LOG(LogTemp, Warning, TEXT("%s died lol noob"), *WhoDied->GetName());

	FTileState* Room = GetTile();
	if (!bIsDoorOpen && Room && Room->Template && Room->Template->bIsDoored && AreAllCharsDead())
	{
		FloorBuilderRef->SetTileCleared(*Room);

		EnableGoals();
		bIsDoorOpen = true;
		// UE_LOG(LogTemp, Warning, TEXT("cleared room at loc %s"), *GetActorLocation().ToString());
	}

}

void ATileMaster::EnableGoals()
{
	for (auto &&Goal : Goals)
	{
		Goal->EnableGoal(true);
	}

	LootComponent->DropRandomItemAtLocation(RewardPosition + GetActorLocation());
	
}