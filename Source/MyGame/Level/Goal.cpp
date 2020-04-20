// Fill out your copyright notice in the Description page of Project Settings.


#include "Goal.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "../Abilities/IGetHit.h"
#include "Engine/World.h"
#include "../MyGameInstance.h"

// Sets default values
AGoal::AGoal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;
	PoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pole"));
	PoleMesh->SetupAttachment(RootComponent);
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flag"));
	FlagMesh->SetupAttachment(RootComponent);
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BoxCollision->SetupAttachment(RootComponent);
	BoxCollision->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	OnActorBeginOverlap.AddDynamic(this, &AGoal::OnGoalBeginOverlap);
}

// Called when the game starts or when spawned
void AGoal::BeginPlay()
{
	Super::BeginPlay();
	EnableGoal(false);
	
}

// Called every frame
void AGoal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGoal::EnableGoal(bool IsEnabled)
{
	PoleMesh->SetVisibility(IsEnabled);
	FlagMesh->SetVisibility(IsEnabled);
	bIsEnabled = IsEnabled;
}


void AGoal::OnGoalBeginOverlap(AActor* OverlappingActor, AActor* OtherActor)
{
	if (!bIsEnabled) return;
	IGetHit *Target = Cast<IGetHit>(OtherActor);
	if (Target && Target->IsAlive())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s Overlapped %s"), *OverlappingActor->GetName(), *OtherActor->GetName());
		UWorld* LeMundi = GetWorld();
		if (!LeMundi) return;
		LeMundi->ServerTravel(NextMapUrl);
		UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
		if (GI)
		{
			GI->LevelDifficulty++;
		}
	}
}