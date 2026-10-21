// One of the HUD's ability buttons (WBP_ActionButton). Below its icon it shows how to press it: the gamepad
// button's picture, or the keyboard keys when the player is on keyboard. The keys come from the input mapping
// context, so a rebound key shows as bound.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "ActionButtonWidget.generated.h"

UCLASS()
class MYGAME_API UActionButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Shows the gamepad pictures (bGamepad) or the keyboard keys
	void ShowInputDevice(bool bGamepad);

	// The ability slot this button shows: the Blueprint's ButtonInput, which each button on the action bar sets
	EInput GetSlot() const;

protected:
	virtual void NativeConstruct() override;

	// Index 0: the gamepad pictures; index 1: the keyboard key text
	UPROPERTY(BlueprintReadOnly, Category = "Action Button", meta = (BindWidgetOptional))
	TObjectPtr<class UWidgetSwitcher> HotkeySwitcher;

	UPROPERTY(BlueprintReadOnly, Category = "Action Button", meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> CommandTextWidget;
};
