// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomMaster.h"
#include "Components/BillboardComponent.h"

#include "../Abilities/LootComponent.h"
#include "../Player/MyCharacter.h"
#include "../Player/ReportDeath.h"
#include "LevelBuilder.h"
#include "RunSubsystem.h"
#include "Goal.h"

// Sets default values
ARoomMaster::ARoomMaster()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Dino"));
	RootComponent = Billboard;

	LootComponent = CreateDefaultSubobject<ULootComponent>(TEXT("Loot Component"));

}

// Called when the game starts or when spawned
void ARoomMaster::BeginPlay()
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
			AddNewCharToRoom(Char);
		}
		AGoal* NewGoal = Cast<AGoal>(Actor);
		if (NewGoal)
		{
			Goals.Add(NewGoal);
		}

	}
	LevelBuilderRef = URunSubsystem::GetLevelBuilder(this);
	if (!LevelBuilderRef)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: no LevelBuilder in this world, room will not be tracked"), *GetName());
		return;
	}
	if (FRoomState* Room = LevelBuilderRef->RegisterRoomMaster(this)) LootComponent->LootTable = Room->RoomReward;
	
}

void ARoomMaster::AddNewCharToRoom(AMyCharacter* Char)
{
	CharsToKill.Add(Char);
	if (!Char)
	{
		UE_LOG(LogTemp, Error, TEXT("Spawner could not bind to Char"));
		return;
	}
	Char->OnDieDelegate.AddUObject(this, &ARoomMaster::OnCharDied);
}

// Called every frame
void ARoomMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FRoomState* ARoomMaster::GetRoomState() const
{
	return LevelBuilderRef ? LevelBuilderRef->GetRoomStateOfLevel(GetLevel()) : nullptr;
}

bool ARoomMaster::AreAllCharsDead()
{
	for (auto &&Char : CharsToKill)
	{
		if (!Char || !LevelBuilderRef) continue;
		bool bIsInMyRoom = (LevelBuilderRef->GetGridFromLoc(Char->GetActorLocation()) == LevelBuilderRef->GetGridFromLoc(GetActorLocation()));
		if (Char->IsAlive() && bIsInMyRoom) return false;
	}
	return true;
}

TArray<class AMyCharacter*> ARoomMaster::GetChars()
{
	return CharsToKill;
}

/// <summary>
/// Aggroes all chars that spawned on this room to te given Player
/// </summary>
/// <param name="PlayerRef">Player to Aggro all chars on</param>
void ARoomMaster::AggroAll(APawn* PlayerRef)
{
	if (bAggroed) return;
	bAggroed = true;
	for (auto&& Enemy : CharsToKill)
	{
		Enemy->SetAggroTarget(PlayerRef);
	}
}

void ARoomMaster::OnCharDied(AMyCharacter* WhoDied)
{
	UE_LOG(LogTemp, Warning, TEXT("%s died lol noob"), *WhoDied->GetName());

	FRoomState* Room = GetRoomState();
	if (!bIsDoorOpen && Room && Room->RoomType && Room->RoomType->bIsDoored && AreAllCharsDead())
	{
		LevelBuilderRef->SetRoomCleared(*Room);

		EnableGoals();
		bIsDoorOpen = true;
		// UE_LOG(LogTemp, Warning, TEXT("cleared room at loc %s"), *GetActorLocation().ToString());
	}

}

void ARoomMaster::EnableGoals()
{
	for (auto &&Goal : Goals)
	{
		Goal->EnableGoal(true);
	}

	LootComponent->DropRandomItemAtLocation(RewardPosition + GetActorLocation());
	
}