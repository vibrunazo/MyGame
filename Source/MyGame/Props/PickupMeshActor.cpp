// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupMeshActor.h"

#include "Components/StaticMeshComponent.h"

// Sets default values
APickupMeshActor::APickupMeshActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComp);
	//Mesh->SetRenderCustomDepth(true);
	//Mesh->SetCustomDepthStencilValue(2);

}

void APickupMeshActor::UpdateMesh()
{
	UE_LOG(LogTemp, Warning, TEXT("update mesh on APickupMeshActor"));
}

void APickupMeshActor::SetMeshOutline()
{
	//UE_LOG(LogTemp, Warning, TEXT("setting mesh outline"));
	if (!Mesh) return;
	Mesh->SetRenderCustomDepth(true);
	Mesh->SetCustomDepthStencilValue(2);
	//UE_LOG(LogTemp, Warning, TEXT("CD %d %d"), Mesh->bRenderCustomDepth, Mesh->CustomDepthStencilValue);
}

// Called when the game starts or when spawned
void APickupMeshActor::BeginPlay()
{
	Super::BeginPlay();
	
}
