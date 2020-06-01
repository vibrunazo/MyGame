// Fill out your copyright notice in the Description page of Project Settings.


#include "Wall.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

// Sets default values
AWall::AWall()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;
	WallMeshes = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Wall Meshes"));
	WallMeshes->SetupAttachment(RootComponent);

}

void AWall::OnConstruction(const FTransform & Transform)
{
	if (!WallMeshes) return;
	WallMeshes->ClearInstances();
	if (!Wall_2m) return;
	WallMeshes->SetStaticMesh(Wall_2m);
	float TileLength = 200.f;
	uint8 NumInstances = FMath::FloorToInt(Length / TileLength);
	for (int i = NumInstances - 1; i >= 0; i--)
	{
		/* code */
		WallMeshes->AddInstance(FTransform(FVector(0.f, -NumInstances*0.5f*TileLength + TileLength*i, 0.f)));
	}
}

// Called when the game starts or when spawned
void AWall::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

