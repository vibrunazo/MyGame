#include "RoomDressing.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "PCGComponent.h"

ARoomDressing::ARoomDressing()
{
	PrimaryActorTick.bCanEverTick = false;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Bounds;
	PCG = CreateDefaultSubobject<UPCGComponent>(TEXT("PCG"));
	PCG->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
}

void ARoomDressing::AddLine(const FVector& From, const FVector& To)
{
	USplineComponent* Line = NewObject<USplineComponent>(this);
	Line->SetupAttachment(RootComponent);
	Line->RegisterComponent();
	Line->SetSplinePoints({ From, To }, ESplineCoordinateSpace::World);
	for (int32 i = 0; i < 2; i++) Line->SetSplinePointType(i, ESplinePointType::Linear);
	Lines.Add(Line);
}

void ARoomDressing::SetRoomExtent(const FVector& HalfSize)
{
	Bounds->SetBoxExtent(HalfSize);
}

void ARoomDressing::Generate(UPCGGraphInterface* Graph, int32 Seed)
{
	if (!Graph || Lines.IsEmpty()) return;
	PCG->SetGraphLocal(Graph);
	PCG->Seed = Seed;
	PCG->GenerateLocal(/*bForce=*/true);
}
