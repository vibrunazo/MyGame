// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner.h"
#include "../MyGameInstance.h"
#include "../Player/MyCharacter.h"


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

// Called when the game starts or when spawned
void ASpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASpawner::SpawnActors()
{
	UE_LOG(LogTemp, Warning, TEXT("spawner is spawning"));

	if (!CharToSpawn) return;

	FVector Loc = GetActorLocation();
	FActorSpawnParameters params;
	params.bNoFail = true;
	params.Owner = this;
	AMyCharacter* NewChar = GetWorld()->SpawnActor<AMyCharacter>(CharToSpawn, Loc, FRotator::ZeroRotator, params);
	if (bAggroOnSpawn)
	{
		UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance());
		AMyCharacter* PlayerRef = nullptr;
		if (MyGI) PlayerRef = MyGI->PlayerCharRef;
		NewChar->SetAggroTarget(PlayerRef);
		UE_LOG(LogTemp, Warning, TEXT("told %s to aggro on %s"), *NewChar->GetName(), *PlayerRef->GetName());
	}
}

