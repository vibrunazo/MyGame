// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MyProjectile.generated.h"

UCLASS()
class MYGAME_API AMyProjectile : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMyProjectile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile)
	class UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile)
	class AHitBox* HitboxRef;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile)
	class UProjectileMovementComponent* MovementComp;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
