// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../BaseController.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API AMyPlayerController : public ABaseController
{
	GENERATED_BODY()

public:
	void OnCharDies(class AMyCharacter* CharRef);
	void OnDelayedCharDies(class AMyCharacter* CharRef);
	void ShowGameOver();
	void OnLevelWin(class AMyCharacter* CharRef);
	void OnDelayedLevelWin(class AMyCharacter* CharRef);
	void ShowLevelCleared();

	class AMyDefaultPawn* DefaultPawnRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> GameOverWidget;
	class UMyUserWidget* GameOverWidgetRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> LevelClearedWidget;
	class UMyUserWidget* LevelClearedWidgetRef;

	bool bIsLevelOver = false;
};
