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
	// UFUNCTION(BlueprintCallable, Category="MyLibrary")
	// TSubclassOf<class UCameraShake> GetCamShakeClass();
	class ALevelBuilder* GetLevelBuilder();
	void SetLevelBuilderRef(class ALevelBuilder* NewLevelBuilder);
	void OnGameOver();
	void LevelClear(FString NextMapUrl);
	void SetCharRef(class AMyCharacter* NewRef);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	TSubclassOf<class UCameraShake> CamShakeClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	TSubclassOf<class UGameplayEffect> StunImmuneEffectRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	uint8 LevelDifficulty = 1;

	float Health = 100.f;
	class AMyCharacter* PlayerCharRef;

private:
	class ALevelBuilder* LevelBuilderRef = nullptr;

};
