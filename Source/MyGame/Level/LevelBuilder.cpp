// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBuilder.h"
#include "Components/BillboardComponent.h"

// Sets default values
ALevelBuilder::ALevelBuilder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BBComp = CreateDefaultSubobject<UBillboardComponent>(TEXT("Dino"));
	RootComponent = BBComp;
	// BBComp->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void ALevelBuilder::BeginPlay()
{
	Super::BeginPlay();
	
}

