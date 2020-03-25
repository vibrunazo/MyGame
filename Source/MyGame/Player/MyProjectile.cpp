// Fill out your copyright notice in the Description page of Project Settings.


#include "MyProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
AMyProjectile::AMyProjectile()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));


}

// Called when the game starts or when spawned
void AMyProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyProjectile::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

