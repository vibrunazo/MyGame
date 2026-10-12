#include "TileDressing.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "PCGComponent.h"

ATileDressing::ATileDressing()
{
	PrimaryActorTick.bCanEverTick = false;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Bounds;
	PCG = CreateDefaultSubobject<UPCGComponent>(TEXT("PCG"));
	PCG->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
}

void ATileDressing::AddLine(const FVector& From, const FVector& To)
{
	USplineComponent* Line = NewObject<USplineComponent>(this);
	Line->SetupAttachment(RootComponent);
	Line->RegisterComponent();
	Line->SetSplinePoints({ From, To }, ESplineCoordinateSpace::World);
	for (int32 i = 0; i < 2; i++) Line->SetSplinePointType(i, ESplinePointType::Linear);
	Lines.Add(Line);
}

void ATileDressing::SetTileExtent(const FVector& HalfSize)
{
	Bounds->SetBoxExtent(HalfSize);
}

void ATileDressing::Generate(UPCGGraphInterface* Graph, int32 Seed)
{
	if (!Graph || Lines.IsEmpty()) return;
	PCG->SetGraphLocal(Graph);
	PCG->Seed = Seed;
	PCG->GenerateLocal(/*bForce=*/true);
}
