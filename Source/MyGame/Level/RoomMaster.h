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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LevelBuilder)
	class ULootComponent* LootComponent;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = LevelBuilder)
	class UBillboardComponent* Billboard;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LevelBuilder, meta = (tooltip = "Chars that needs to be killed for the room to be considered clear and open its door. If empty, will automatically fill it with all actors found inside the room on Begin Play."))
	TArray<class AMyCharacter*> CharsToKill;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelBuilde, meta = (MakeEditWidget = true, tooltip = "Position in the Room where the rewards items should spawn when the room is cleared."))
	FVector RewardPosition = FVector();
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
