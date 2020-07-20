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
	void PickRandomSpawnIndex();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	class UBoxComponent* BoxComp;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	class UBillboardComponent* BBComp;

	// Will Randomly spawn one of these Actors when SpawnActors() is called
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TArray<TSubclassOf<class AMyCharacter>> CharsToSpawn;
	// If true, Spawned actors will automatically aggro on the Player Char when spawned
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bAggroOnSpawn = true;
	// If true, will automatically Spawn on Begin Play. Else someone else will need to manually call SpawnActors() on me
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bSpawnOnBegin = true;

private:
	uint8 IndexToSpawn = 0;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


};
