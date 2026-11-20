#include "MyPatternStepper.h"
#include "MyAIController.h"
#include "EngineUtils.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	// its path following too: a move the pattern starts this step is followed this frame, in every world
	if (UPathFollowingComponent* Following = Controller->GetPathFollowingComponent())
	{
		Following->PrimaryComponentTick.AddPrerequisite(Stepper, Stepper->PrimaryActorTick);
	}

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

void AMyPatternStepper::FollowTarget(UWorld* World, const ACharacter* Target)
{
	AMyPatternStepper* Stepper = Target ? FindOrMake(World) : nullptr;
	if (!Stepper) return;
	// Every tick that can move the target before the stepper's group ends: its own, its movement's and its components'
	// (the combat component steps a backslide the movement didn't take, and steps out of other characters)
	ACharacter* Moved = const_cast<ACharacter*>(Target);
	if (Moved->PrimaryActorTick.bCanEverTick && Moved->PrimaryActorTick.TickGroup <= Stepper->PrimaryActorTick.TickGroup)
	{
		Stepper->PrimaryActorTick.AddPrerequisite(Moved, Moved->PrimaryActorTick);
	}
	for (UActorComponent* Component : Moved->GetComponents())
	{
		FActorComponentTickFunction& Tick = Component->PrimaryComponentTick;
		if (Tick.bCanEverTick && Tick.TickGroup <= Stepper->PrimaryActorTick.TickGroup) Stepper->PrimaryActorTick.AddPrerequisite(Component, Tick);
	}
}
