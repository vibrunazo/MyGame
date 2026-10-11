// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MyBlueprintFunctionLibrary.h"
#include "MyGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UMyGameInstance();

	void Init() override;

	// The run's flow: menu -> StartNewRun -> NextFloor after each boss -> death or quit -> menu
	UFUNCTION(BlueprintCallable, Category="MyGI")
	void StartNewRun();
	// The floor is cleared: one floor harder, same run (health, mana, items and learned moves carry over)
	UFUNCTION(BlueprintCallable, Category="MyGI")
	void NextFloor();
	UFUNCTION(BlueprintCallable, Category="MyGI")
	void QuitToMainMenu();
	// Resets the run state (floor 1, full health, no items or learned moves, a new seed)
	void NewGame();
	// What carries over to the next floor, taken from the player when the floor is cleared
	void StoreCharStats();
	void SetCharRef(class AMyCharacter* NewRef);
	TArray<FString> GetItemsICannotGetMoreOf();
	TArray<struct FAbilityStruct> GetPlayerAbiliies();
	void TeleportPlayer(FVector NewLocation);
	void DoCamShake(float Intensity);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	TSubclassOf<class UCameraShakeBase> CamShakeClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	TSubclassOf<class UGameplayEffect> StunImmuneEffectRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	TSubclassOf<class UGameplayEffect> NoControlEffectRef;

	// The generated level a run plays on, and the main menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MyGI)
	TSoftObjectPtr<UWorld> RunMap;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MyGI)
	TSoftObjectPtr<UWorld> MainMenuMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	uint8 LevelDifficulty = 0;
	UPROPERTY(VisibleInstanceOnly, Category = MyGI)
	float Health = 100.f;
	UPROPERTY(VisibleInstanceOnly, Category = MyGI)
	float Mana = 100.f;
	// The player's moves, learned books included; empty until the first floor is cleared
	UPROPERTY(VisibleInstanceOnly, Category = MyGI)
	TArray<struct FAbilityStruct> CarriedAbilities;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<class UItemDataAsset*> Inventory;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite)
	class AMyCharacter* PlayerCharRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	int32 RandomSeed = 0;
	UPROPERTY(VisibleInstanceOnly, Category = MyGI)
	FRandomStream RandomStream;
};
