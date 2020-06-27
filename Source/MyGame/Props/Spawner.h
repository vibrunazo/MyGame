// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spawner.generated.h"

UCLASS()
class MYGAME_API ASpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawner();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Spawns the actors
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SpawnActors();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	class UBoxComponent* BoxComp;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	class UBillboardComponent* BBComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<class AMyCharacter> CharToSpawn;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bAggroOnSpawn = true;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


};
