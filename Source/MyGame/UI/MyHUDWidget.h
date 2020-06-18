// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyUserWidget.h"
#include "MyHUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UMyHUDWidget : public UMyUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "My User Widget")
	void BPUpdateHUD(AMyCharacter* Char);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "My User Widget")
	void BPUpdateHealth(float NewHealth);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "My User Widget")
	void BPUpdateMana(float NewMana);
	
};
