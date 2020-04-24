// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Goal.generated.h"

UCLASS()
class MYGAME_API AGoal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGoal();

	UFUNCTION(BlueprintCallable, Category = Goal)
	void EnableGoal(bool IsEnabled);

	UFUNCTION()
	void OnGoalBeginOverlap(AActor* OverlappingActor, AActor* OtherActor);
	UFUNCTION()
	void OnDelayedOpenLevel();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Goal)
	class USceneComponent* RootComp;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Goal)
	class UStaticMeshComponent* PoleMesh;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Goal)
	class UStaticMeshComponent* FlagMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Goal)
	class UBoxComponent* BoxCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Goal)
	FString NextMapUrl = "";
private:
	bool bIsEnabled = false;
	bool bIsCleared = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
