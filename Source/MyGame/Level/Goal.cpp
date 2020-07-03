// Fill out your copyright notice in the Description page of Project Settings.


#include "Goal.h"
#include "../MyGameInstance.h"
#include "../Abilities/IGetHit.h"
#include "../Player/MyCharacter.h"
#include "../Player/MyPlayerController.h"

#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
// #include "Engine/World.h"
#include "TimerManager.h"

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
	GoalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Goal Light"));
	GoalLight->SetupAttachment(RootComponent);
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
	GoalLight->SetVisibility(IsEnabled);
	bIsEnabled = IsEnabled;
}


void AGoal::OnGoalBeginOverlap(AActor* OverlappingActor, AActor* OtherActor)
{
	if (!bIsEnabled && !bIsCleared) return;
	AMyCharacter *Target = Cast<AMyCharacter>(OtherActor);
	if (Target && Target->IsAlive() && Target->IsPlayerControlled())
	{
		// UE_LOG(LogTemp, Warning, TEXT("%s Overlapped %s"), *OverlappingActor->GetName(), *OtherActor->GetName());
		bIsCleared = true;
		AMyPlayerController* Cont = Cast<AMyPlayerController>(Target->GetController());
		if (Cont) Cont->OnLevelWin(Target);
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, this, &AGoal::OnDelayedOpenLevel, 5.0f, false);
	}
}

void AGoal::OnDelayedOpenLevel()
{
	UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
	if (GI)
	{
		GI->LevelClear(NextMapUrl);
	}
}