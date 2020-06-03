// Fill out your copyright notice in the Description page of Project Settings.


#include "Wall.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"

// Sets default values
AWall::AWall()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;
	WallMeshes = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Wall Meshes"));
	WallMeshes->SetupAttachment(RootComponent);
	CollisionTop = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Top"));
	CollisionTop->SetupAttachment(RootComponent);
	CollisionTop->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	CollisionTop->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	CollisionTop->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionTop->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CollisionLeft = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Left"));
	CollisionLeft->SetupAttachment(RootComponent);
	CollisionLeft->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	CollisionLeft->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	CollisionLeft->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionLeft->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CollisionRight = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Right"));
	CollisionRight->SetupAttachment(RootComponent);
	CollisionRight->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	CollisionRight->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	CollisionRight->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionRight->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

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
	if (!WallMesh) return;
	WallMeshes->SetStaticMesh(WallMesh);
	float TileLength = 100.f;
	float TileHeight = 100.f;
	float TileDepth = 100.f;
	float CollisionHeight = 800.f;
	FTransform TrTop = FTransform(FVector(0.f, 0.f, 0.f));
	FTransform TrColTop = FTransform();
	if (bIsDoored)
	{
		if (Height > 200.f)
		{
			TrTop.SetScale3D(FVector(Depth/TileDepth, Length / TileLength, ((Height - 200.f) / TileHeight)));
			TrTop.SetLocation(FVector(0.f, 0.f, Height/2 + 100.f));
			WallMeshes->AddInstance(TrTop);
			CollisionLeft->SetRelativeTransform(FTransform(FVector(0.f, 0.f, 0.f)));
			CollisionRight->SetRelativeTransform(FTransform(FVector(0.f, 0.f, 0.f)));
		}
		if (Length > 200.f)
		{
			FTransform TrLeft = FTransform(FVector(0.f, -(Length - 200.f)/4.f - 100.f, FMath::Min(Height/2, 100.f)));
			FTransform TrRight = FTransform(FVector(0.f, (Length - 200.f)/4.f + 100.f, FMath::Min(Height/2, 100.f)));
			TrLeft.SetScale3D(FVector(Depth/TileDepth, ((Length - 200.f)/2.f) / TileLength, (FMath::Min(Height, 200.f) / TileHeight)));
			TrRight.SetScale3D(FVector(Depth/TileDepth, ((Length - 200.f)/2.f) / TileLength, (FMath::Min(Height, 200.f) / TileHeight)));
			WallMeshes->AddInstance(TrLeft);
			WallMeshes->AddInstance(TrRight);
		}
		FTransform TrColLeft = FTransform(FVector(0.f, -(Length - 200.f)/4.f - 100.f, 100.f));
		FTransform TrColRight = FTransform(FVector(0.f, (Length - 200.f)/4.f + 100.f, 100.f));
		TrColLeft.SetScale3D(FVector(Depth/TileDepth, ((Length - 200.f)/2.f) / TileLength, 200.f / TileHeight));
		TrColRight.SetScale3D(FVector(Depth/TileDepth, ((Length - 200.f)/2.f) / TileLength, 200.f / TileHeight));
		TrColTop.SetScale3D(FVector(Depth/TileDepth, Length / TileLength, ((CollisionHeight - 200.f) / TileHeight)));
		TrColTop.SetLocation(FVector(0.f, 0.f, CollisionHeight/2 + 100.f));
		CollisionTop->SetRelativeTransform(TrColTop);
		CollisionLeft->SetRelativeTransform(TrColLeft);
		CollisionRight->SetRelativeTransform(TrColRight);
		CollisionLeft->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionRight->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		TrTop.SetScale3D(FVector(Depth/TileDepth, Length / TileLength, Height / TileHeight));
		TrTop.SetLocation(FVector(0.f, 0.f, Height/2));
		TrColTop = FTransform();
		TrColTop.SetScale3D(FVector(Depth/TileDepth, Length / TileLength, CollisionHeight / TileHeight));
		TrColTop.SetLocation(FVector(0.f, 0.f, CollisionHeight/2));
		WallMeshes->AddInstance(TrTop);
		CollisionTop->SetRelativeTransform(TrColTop);
		CollisionLeft->SetRelativeTransform(FTransform());
		CollisionRight->SetRelativeTransform(FTransform());
		CollisionLeft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionRight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

}
