// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomMaster.h"
#include "Components/BillboardComponent.h"
#include "../Player/MyCharacter.h"
#include "../MyGameInstance.h"
#include "LevelBuilder.h"

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
			UE_LOG(LogTemp, Warning, TEXT("Found Char: %s"), *Actor->GetName());
			CharsToKill.Add(Char);
		}

	}
	UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance());
	if (!ensure(MyGI != nullptr)) return;
	ALevelBuilder* LBuilder = MyGI->GetLevelBuilder();
	LevelBuilderRef = LBuilder;
	bIsDoorOpen = false;
	
}

// Called every frame
void ARoomMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// UE_LOG(LogTemp, Warning, TEXT("are all chars dead: %d"), AreAllCharsDead());
	if (!bIsDoorOpen && AreAllCharsDead())
	{
		LevelBuilderRef->SetRoomClearedAtLoc(GetActorLocation());
		bIsDoorOpen = true;
	}

}

bool ARoomMaster::AreAllCharsDead()
{
	for (auto &&Char : CharsToKill)
	{
		if (Char->IsAlive()) return false;
	}
	return true;
}