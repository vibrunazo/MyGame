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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	TSubclassOf<class UCameraShake> CamShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyGI)
	uint8 LevelDifficulty = 1;

private:
	class ALevelBuilder* LevelBuilderRef = nullptr;

};
