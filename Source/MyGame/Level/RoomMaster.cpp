// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomMaster.h"
#include "Components/BillboardComponent.h"
#include "../Player/MyCharacter.h"
#include "../MyGameInstance.h"
#include "LevelBuilder.h"
#include "Goal.h"

// Sets default values
ARoomMaster::ARoomMaster()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Dino"));
	RootComponent = Billboard;

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
		AMyCharacter* Char = Cast<AMyCharacter>(Actor);
		if (Char)
		{
			CharsToKill.Add(Char);
			Char->OnDieDelegate.AddDynamic(this, &ARoomMaster::OnCharDied);
		}
		AGoal* NewGoal = Cast<AGoal>(Actor);
		if (NewGoal)
		{
			Goals.Add(NewGoal);
		}

	}
	UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance());
	if (!ensure(MyGI != nullptr)) return;
	ALevelBuilder* LBuilder = MyGI->GetLevelBuilder();
	LevelBuilderRef = LBuilder;
	LevelBuilderRef->RegisterRoomMaster(this, GetActorLocation());
	
}

// Called every frame
void ARoomMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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

	if (!bIsDoorOpen && RoomStateRef && RoomStateRef->RoomType->bIsDoored && AreAllCharsDead())
	{
		if (!ensure(LevelBuilderRef != nullptr)) return;
		LevelBuilderRef->SetRoomClearedAtLoc(GetActorLocation());

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
	
}