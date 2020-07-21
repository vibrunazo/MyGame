// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomMaster.generated.h"

UCLASS()
class MYGAME_API ARoomMaster : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARoomMaster();

	bool AreAllCharsDead();
	TArray<class AMyCharacter*> GetChars();
	void AggroAll(APawn* PlayerRef);
	void AddNewCharToRoom(AMyCharacter* Char);
	UFUNCTION()
	void OnCharDied(class AMyCharacter* WhoDied);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = LevelBuilder)
	class UBillboardComponent* Billboard;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LevelBuilder)
	TArray<class AMyCharacter*> CharsToKill;
	UPROPERTY()
	TArray<class AGoal*> Goals;

	bool bIsDoorOpen = true;
	bool bAggroed = false;
	UPROPERTY()
	class ALevelBuilder* LevelBuilderRef = nullptr;
	struct FRoomState* RoomStateRef = nullptr;

private:
	void EnableGoals();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
