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
	BuildWalls();
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

void AWall::BuildWalls()
{

	if (!WallMeshes) return;
	WallMeshes->ClearInstances();
	if (!Wall_2m) return;
	WallMeshes->SetStaticMesh(Wall_2m);
	float TileLength = 200.f;
	float TileHeight = 200.f;
	FTransform TrTop = FTransform(FVector(0.f, 0.f, 0.f));
	if (bIsDoored)
	{
		if (Height > 200.f)
		{
			TrTop.SetScale3D(FVector(1.f, Length / TileLength, ((Height - 200.f) / TileHeight)));
			TrTop.SetLocation(FVector(0.f, 0.f, 200.f));
			WallMeshes->AddInstance(TrTop);
		}
		if (Length > 200.f)
		{
			FTransform TrLeft = FTransform(FVector(0.f, -(Length - 200.f)/4.f - 100.f, 0.f));
			FTransform TrRight = FTransform(FVector(0.f, (Length - 200.f)/4.f + 100.f, 0.f));
			TrLeft.SetScale3D(FVector(1.f, ((Length - 200.f)/2.f) / TileLength, (FMath::Min(Height, 200.f) / TileHeight)));
			TrRight.SetScale3D(FVector(1.f, ((Length - 200.f)/2.f) / TileLength, (FMath::Min(Height, 200.f) / TileHeight)));
			WallMeshes->AddInstance(TrLeft);
			WallMeshes->AddInstance(TrRight);
		}
	}
	else
	{
		TrTop.SetScale3D(FVector(1.f, Length / TileLength, Height / TileHeight));
		WallMeshes->AddInstance(TrTop);
	}
}
