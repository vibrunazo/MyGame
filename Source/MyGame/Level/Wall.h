// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Wall.generated.h"

UCLASS()
class MYGAME_API AWall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWall();
	void OnConstruction(const FTransform & Transform) override;

	void BuildWalls();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Wall)
	class USceneComponent* RootComp;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Wall)
	class UHierarchicalInstancedStaticMeshComponent* WallMeshes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wall)
	float Length = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wall)
	float Height = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wall)
	bool bIsDoored = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wall)
	class UStaticMesh* Wall_2m;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wall)
	// class UStaticMesh* Wall_Tall2m;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
