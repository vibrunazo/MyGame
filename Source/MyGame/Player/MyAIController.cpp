// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAIController.h"
#include "MyCharacter.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AMyAIController::AMyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	// The values every enemy's old PawnSensing used: 700 units, 90 degrees to each side, players only (filtered below)
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 700.f;
	SightConfig->LoseSightRadius = 800.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	Perception->ConfigureSense(*SightConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*Perception);
}

void AMyAIController::BeginPlay()
{
	Super::BeginPlay();
	if (UAIPerceptionComponent* Perception = GetAIPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &AMyAIController::OnTargetPerceived);
	}
}

void AMyAIController::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	APawn* SeenPawn = Cast<APawn>(Actor);
	if (!Stimulus.WasSuccessfullySensed() || !SeenPawn || !SeenPawn->IsPlayerControlled()) return;
	if (AMyCharacter* Me = GetPawn<AMyCharacter>()) Me->OnPawnSeen(SeenPawn);
}
