// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyUserWidget.h"
#include "MainMenu.generated.h"

/**
 * A menu (the main menu and the pause menu): its first button takes keyboard and gamepad focus when it's shown.
 */
UCLASS()
class MYGAME_API UMainMenu : public UMyUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
};
