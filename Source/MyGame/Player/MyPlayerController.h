// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../BaseController.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

// A button and the ability slot it presses. The AI presses the same slots through the character.
USTRUCT(BlueprintType)
struct FAbilityInputBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UInputAction> Action = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EInput Slot = EInput::Punch;
};

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
	void SetAbilityKeyDown(EInput Index, bool IsKeyDown);
	UFUNCTION(BlueprintCallable, Category = BaseController)
	bool IsAbilityKeyDown(uint8 Index);
	UFUNCTION(BlueprintCallable, Category = BaseController)
	void UpdateHUDAbilityKey(EInput Index, bool IsKeyDown, float Duration = 0.0f);
	UFUNCTION(BlueprintCallable, Category = BaseController)
	void ShowAbilityCooldown(uint8 Index, float Cooldown);
	UFUNCTION(BlueprintCallable, Category = BaseController)
	void UpdateHUDAbility(FAbilityStruct Ability, bool NewState);
	// Every move the character knows on the HUD (a later floor starts with carried moves, not the defaults)
	void RefreshHUDAbilities(class AMyCharacter* Char);
	UFUNCTION(BlueprintCallable, Category = BaseController)
	void SetSuperMod(bool NewState);
	UFUNCTION(BlueprintCallable, Category = BaseController)
	bool GetSuperMod();
	UFUNCTION(BlueprintCallable, Category = BaseController)
	void SetUltraMod(bool NewState);
	UFUNCTION(BlueprintCallable, Category = BaseController)
	bool GetUltraMod();
	UFUNCTION(BlueprintCallable, Category = BaseController)
	uint8 GetModValue();
	// Whether the player last used a gamepad (the HUD shows its buttons) or the keyboard (the HUD shows the keys)
	UFUNCTION(BlueprintPure, Category = Input)
	bool IsUsingGamepad() const { return bUsingGamepad; }
	// The keyboard keys that press Slot as the HUD shows them ("←", "Shift + ↑", "WASD"), from the default mapping
	// context; empty if no key presses it
	UFUNCTION(BlueprintPure, Category = Input)
	FText GetKeyboardText(EInput Slot) const;
	// The local player's Enhanced Input subsystem (mapping contexts, key remapping, input injection in tests)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Input)
	class UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;
#if WITH_EDITOR
	// PIE test hook: holds an input action at Value (as a key or stick would) until called with bHeld = false
	UFUNCTION(BlueprintCallable, Category = "Testing")
	void InjectTestInput(UInputAction* Action, FVector2D Value, bool bHeld);
	// PIE test hook: switches the HUD as if the player had pressed a gamepad button (true) or a key (false)
	UFUNCTION(BlueprintCallable, Category = "Testing")
	void SetUsingGamepadForTest(bool bGamepad) { SetUsingGamepad(bGamepad); }
	// PIE test hook: the input mode last set ("Game Only", "UI Only", ...); real keys and pads only reach the game in Game Only
	UFUNCTION(BlueprintCallable, Category = "Testing")
	FString GetInputModeForTest() const { return GetCurrentInputModeDebugString(); }
#endif

	UPROPERTY()
	TObjectPtr<class AMyDefaultPawn> DefaultPawnRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> GameOverWidget;
	UPROPERTY()
	TObjectPtr<class UMyUserWidget> GameOverWidgetRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> LevelClearedWidget;
	UPROPERTY()
	TObjectPtr<class UMyUserWidget> LevelClearedWidgetRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> PauseWidget;
	UPROPERTY()
	TObjectPtr<class UMyUserWidget> PauseWidgetRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyHUDWidget> HUDWidget;
	UPROPERTY()
	TObjectPtr<class UMyHUDWidget> HUDWidgetRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> IntroWidget;
	UPROPERTY()
	TObjectPtr<class UMyUserWidget> IntroWidgetRef;

	bool bIsLevelOver = false;
	bool bIsPaused = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	// 2D: X = forward (world +X), Y = right (world +Y)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TArray<FAbilityInputBinding> AbilityInputs;
	// Same keys as MoveAction with a Directional Double Tap trigger
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> DashAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> SuperModAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> UltraModAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> PauseAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> ShowFPSAction;


private:
	TArray<bool> AbilityKeyStates = { false, false, false, false, false, false, false, false };
	bool bSuperMod = false;
	bool bUltraMod = false;
	bool bUsingGamepad = false;
	// Switches the HUD's buttons between gamepad pictures and keyboard keys
	void SetUsingGamepad(bool bGamepad);
	// The keyboard keys mapped to Action: one key's name, or a group of letter keys in keyboard order ("WASD")
	FText KeyboardKeysText(const UInputAction* Action) const;

protected:
	void BeginPlay() override;
	virtual void SetupInputComponent() override;
	// Every key, button and stick reaches the controller here first: the last one pressed says which device is in use
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;
	void Jump();
	void StopJump();
	// The pawn while it takes input: null after death or the level-clear flag (both clear bHasControl)
	class AMyCharacter* GetControlledChar() const;
	void OnMove(const struct FInputActionValue& Value);
	void OnDash(const struct FInputActionValue& Value);
	void OnAbilityInput(EInput Slot, bool bPressed);
	void OnShowFPS();
};
