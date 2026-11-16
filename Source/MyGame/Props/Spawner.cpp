// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner.h"
#include "../MyGameInstance.h"
#include "../Player/MyCharacter.h"
#include "../Level/TileMaster.h"


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

void ASpawner::TryToUpdateTileMasterRef()
{
	// the room master that shares this spawner's room map (it may not have begun play yet)
	for (AActor* Actor : GetLevel()->Actors)
	{
		if (ATileMaster* Master = Cast<ATileMaster>(Actor))
		{
			TileMasterRef = Master;
			return;
		}
	}
	UE_LOG(LogTemp, Error, TEXT("failed to update room master ref on %s"), *GetName());
}

void ASpawner::RegisterNewCharOnTileMaster(AMyCharacter* NewChar)
{
	if (!TileMasterRef) TryToUpdateTileMasterRef();
	if (!TileMasterRef) return;
	TileMasterRef->AddNewCharToTile(NewChar);
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
	RegisterNewCharOnTileMaster(NewChar);
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
		// A stream of its own, from the floor's seed and where this spawner stands: the tiles stream in (and their
		// spawners begin play) in an order that varies run to run, so drawing from the shared stream gave a seed
		// different enemies each time
		const FVector Loc = GetActorLocation();
		const FIntVector Where(FMath::RoundToInt(Loc.X), FMath::RoundToInt(Loc.Y), FMath::RoundToInt(Loc.Z));
		FRandomStream Stream((int32)HashCombine(GetTypeHash(MyGI->RandomStream.GetInitialSeed()), GetTypeHash(Where)));
		IndexToSpawn = Stream.RandRange(0, CharsToSpawn.Num() - 1);
	}
}
