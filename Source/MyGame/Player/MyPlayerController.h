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
	void OnPausePressed();
	void ShowHUD();
	void ShowLevelIntro();
	void UpdateHUD(AMyCharacter* Char);
	float GetHUDHealth();
	UFUNCTION(BlueprintCallable, Category = BaseController)
	void SetAbilityKeyDown(uint8 Index, bool IsKeyDown, float Duration = 0.0f);
	UFUNCTION(BlueprintCallable, Category = BaseController)
	bool IsAbilityKeyDown(uint8 Index);
	UFUNCTION(BlueprintCallable, Category = BaseController)
	void ShowAbilityCooldown(uint8 Index, float Cooldown);

	class AMyDefaultPawn* DefaultPawnRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> GameOverWidget;
	class UMyUserWidget* GameOverWidgetRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> LevelClearedWidget;
	class UMyUserWidget* LevelClearedWidgetRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> PauseWidget;
	class UMyUserWidget* PauseWidgetRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyHUDWidget> HUDWidget;
	class UMyHUDWidget* HUDWidgetRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> IntroWidget;
	class UMyUserWidget* IntroWidgetRef;

	bool bIsLevelOver = false;
	bool bIsPaused = false;


private:
	TArray<bool> AbilityKeyStates = { false, false, false, false, false, false, false, false };

protected:
	void BeginPlay() override;
	virtual void SetupInputComponent() override;
	void Jump();
	void StopJump();
};
