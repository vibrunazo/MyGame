// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="MyGI")
	class ALevelBuilder* GetLevelBuilder();
	void SetLevelBuilderRef(class ALevelBuilder* NewLevelBuilder);
	UFUNCTION(BlueprintCallable, Category="MyGI")
	void OnGameOver();
	UFUNCTION(BlueprintCallable, Category="MyGI")
	void NewGame();
	UFUNCTION(BlueprintCallable, Category="MyGI")
	void LevelClear(FString NextMapUrl);
	void StoreCharStats();
	void SetCharRef(class AMyCharacter* NewRef);
	TArray<FString> GetItemsICannotGetMoreOf();
	void TeleportPlayer(FVector NewLocation);
	void DoCamShake(float Intensity);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	TSubclassOf<class UCameraShake> CamShakeClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	TSubclassOf<class UGameplayEffect> StunImmuneEffectRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	TSubclassOf<class UGameplayEffect> NoControlEffectRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	uint8 LevelDifficulty = 0;
	UPROPERTY()
	float Health = 100.f;
	UPROPERTY()
	float Mana = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<class UItemDataAsset*> Inventory;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite)
	class AMyCharacter* PlayerCharRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	int32 RandomSeed = 0;
	UPROPERTY()
	FRandomStream RandomStream;

private:
	UPROPERTY()
	class ALevelBuilder* LevelBuilderRef = nullptr;

};
