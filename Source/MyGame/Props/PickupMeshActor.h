// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupMeshActor.generated.h"

UCLASS()
class MYGAME_API APickupMeshActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickupMeshActor();

	void UpdateMesh();
	void SetMeshOutline();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Item Pickup")
	class USceneComponent* RootComp;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Item Pickup")
	class UStaticMeshComponent* Mesh;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
