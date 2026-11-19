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

int32 AMyPatternStepper::Register(AMyAIController* Controller)
{
	UWorld* World = Controller ? Controller->GetWorld() : nullptr;
	if (!World) return 0;
	AMyPatternStepper* Stepper = nullptr;
	for (TActorIterator<AMyPatternStepper> It(World); It; ++It)
	{
		Stepper = *It;
		break;
	}
	if (!Stepper)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Stepper = World->SpawnActor<AMyPatternStepper>(Params);
	}
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
	const UMyGameInstance* Instance = Cast<UMyGameInstance>(World->GetGameInstance());
	const int32 RunSeed = Instance ? Instance->RandomSeed : 0;
	return static_cast<int32>(HashCombine(HashCombine(GetTypeHash(RunSeed), GetTypeHash(Tile)), GetTypeHash(Place)));
}
