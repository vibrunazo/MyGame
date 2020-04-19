// Fill out your copyright notice in the Description page of Project Settings.


#include "Door.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

// Sets default values
ADoor::ADoor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = MyRoot;
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BoxCollision->SetupAttachment(RootComponent);
	BoxCollision->SetBoxExtent(FVector(20.0f, 100.0f, 100.0f));
	BoxCollision->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	BoxCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door Mesh"));
	DoorMesh->SetupAttachment(RootComponent);
	DoorMesh->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

// Called when the game starts or when spawned
void ADoor::BeginPlay()
{
	Super::BeginPlay();

	
}

// Called every frame
void ADoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADoor::OpenDoor()
{
	if (bIsDoorOpen) return;
	bIsDoorOpen = true;
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DoorMesh->AddLocalOffset(FVector(0.f, 0.f, 150.f));
	OnOpenDoorBP();
	UE_LOG(LogTemp, Warning, TEXT("Opening Door"));
}

void ADoor::CloseDoor()
{
	if (!bIsDoorOpen) return;
	bIsDoorOpen = false;
	UE_LOG(LogTemp, Warning, TEXT("Closing Door"));
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DoorMesh->AddLocalOffset(FVector(0.f, 0.f, -150.f));

	OnCloseDoorBP();
}