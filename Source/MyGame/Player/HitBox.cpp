// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "HitBox.h"

// Sets default values
AHitBox::AHitBox()
{
	PrimaryActorTick.bCanEverTick = true;

	MyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = MyRoot;
	MySphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Collision"));
	MySphere->SetupAttachment(RootComponent);
	// MySphere->SetSphereRadius

}

// Called when the game starts or when spawned
void AHitBox::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHitBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

