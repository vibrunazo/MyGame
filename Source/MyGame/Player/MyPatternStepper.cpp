#include "MyPatternStepper.h"
#include "MyAIController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "../MyGameInstance.h"
#include "../Level/RunSubsystem.h"
#include "../Level/FloorBuilder.h"

AMyPatternStepper::AMyPatternStepper()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetCanBeDamaged(false);
}

void AMyPatternStepper::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Controllers.RemoveAll([](const TWeakObjectPtr<AMyAIController>& Each) { return !Each.IsValid(); });
	// Copied: a step may spawn an enemy (a summon), which registers at the end, for the next frame
	const TArray<TWeakObjectPtr<AMyAIController>> InOrder = Controllers;
	for (const TWeakObjectPtr<AMyAIController>& Each : InOrder)
	{
		if (AMyAIController* Controller = Each.Get()) Controller->StepPattern();
	}
}

AMyPatternStepper* AMyPatternStepper::FindOrMake(UWorld* World)
{
	if (!World) return nullptr;
	if (TActorIterator<AMyPatternStepper> It(World); It) return *It;
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	AMyPatternStepper* Stepper = World->SpawnActor<AMyPatternStepper>(Params);
	const UMyGameInstance* Instance = Cast<UMyGameInstance>(World->GetGameInstance());
	if (Stepper && Instance) Stepper->RunSeed = Instance->RandomSeed;
	return Stepper;
}

int32 AMyPatternStepper::Register(AMyAIController* Controller)
{
	AMyPatternStepper* Stepper = FindOrMake(Controller ? Controller->GetWorld() : nullptr);
	if (!Stepper) return 0;
	Stepper->Controllers.AddUnique(Controller);
	Controller->AddTickPrerequisiteActor(Stepper);

	// The salt: its place in its tile (where it was possessed: where it spawned), the tile and the run's seed
	FIntPoint Tile(0, 0);
	const APawn* Pawn = Controller->GetPawn();
	if (AFloorBuilder* Builder = URunSubsystem::GetFloorBuilder(Controller); Builder && Pawn)
	{
		const FTileCoord Coord = Builder->GetTileFromLoc(Pawn->GetActorLocation());
		Tile = FIntPoint(Coord.X, Coord.Y);
	}
	const int32 Place = Stepper->InTile.FindOrAdd(Tile)++;
	return static_cast<int32>(HashCombine(HashCombine(GetTypeHash(Stepper->RunSeed), GetTypeHash(Tile)), GetTypeHash(Place)));
}

void AMyPatternStepper::ResetPlaces(UWorld* World, int32 RunSeed)
{
	if (AMyPatternStepper* Stepper = FindOrMake(World))
	{
		Stepper->InTile.Reset();
		Stepper->RunSeed = RunSeed;
	}
}
