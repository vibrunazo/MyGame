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

	UFUNCTION(BlueprintCallable, Category="MyGI")
	class ALevelBuilder* GetLevelBuilder();
	void SetLevelBuilderRef(class ALevelBuilder* NewLevelBuilder);
	UFUNCTION(BlueprintCallable, Category="MyGI")
	void OnGameOver();
	UFUNCTION(BlueprintCallable, Category="MyGI")
	void NewGame();
	UFUNCTION(BlueprintCallable, Category="MyGI")
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
