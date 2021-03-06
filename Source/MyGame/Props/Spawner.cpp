// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner.h"
#include "../MyGameInstance.h"
#include "../Player/MyCharacter.h"
#include "../Level/LevelBuilder.h"
#include "../Level/RoomMaster.h"


#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"

// Sets default values
ASpawner::ASpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BBComp = CreateDefaultSubobject<UBillboardComponent>(TEXT("Dino"));

	RootComponent = BoxComp;
	BBComp->SetupAttachment(RootComponent);

	BoxComp->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);

}

void ASpawner::TryToUpdateRoomMasterRef()
{
	UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance());
	if (MyGI)
	{
		ALevelBuilder* LB = MyGI->GetLevelBuilder();
		if (LB)
		{
			FRoomState* RS = LB->GetRoomStateFromLoc(GetActorLocation());
			if (RS)
			{
				RoomMasterRef = RS->RoomMasterRef;
				return;
			}
		}
	}
	UE_LOG(LogTemp, Error, TEXT("failed to update room master ref on %s"), *GetName());
}

void ASpawner::RegisterNewCharOnRoomMaster(AMyCharacter* NewChar)
{
	if (!RoomMasterRef) TryToUpdateRoomMasterRef();
	if (!RoomMasterRef) return;
	RoomMasterRef->AddNewCharToRoom(NewChar);
}

// Called when the game starts or when spawned
void ASpawner::BeginPlay()
{
	Super::BeginPlay();
	PickRandomSpawnIndex();
	if (bSpawnOnBegin) SpawnActors();
}

// Called every frame
void ASpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASpawner::SpawnActors()
{
	UE_LOG(LogTemp, Warning, TEXT("spawner is spawning"));

	if (CharsToSpawn.Num() == 0) return;

	FVector Loc = GetActorLocation();
	FActorSpawnParameters params;
	params.bNoFail = true;
	params.Owner = this;
	AMyCharacter* NewChar = GetWorld()->SpawnActor<AMyCharacter>(CharsToSpawn[IndexToSpawn], Loc, GetActorRotation(), params);
	RegisterNewCharOnRoomMaster(NewChar);
	if (bAggroOnSpawn)
	{
		UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance());
		AMyCharacter* PlayerRef = nullptr;
		if (MyGI) PlayerRef = MyGI->PlayerCharRef;
		NewChar->SetAggroTarget(PlayerRef);
	}
}

void ASpawner::PickRandomSpawnIndex()
{
	if (CharsToSpawn.Num() == 0) return;
	IndexToSpawn = 0;
	UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance());
	if (MyGI)
	{
		IndexToSpawn = MyGI->RandomStream.RandRange(0, CharsToSpawn.Num() - 1);
	}
}

