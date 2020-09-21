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
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "My HUD")
	void BPUpdateHUD(AMyCharacter* Char);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "My HUD")
	void BPUpdateHealth(float NewHealth);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "My HUD")
	void BPUpdateMana(float NewMana);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "My HUD")
	void BPUpdateAbilityKey(uint8 Index, bool IsKeyDown, float Duration = 0.0f);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "My HUD")
	void BPUpdateCooldown(uint8 Index, float Cooldown);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "My HUD")
	void BPUpdateActionSlot(EInput Index, bool NewState);
	UFUNCTION(BlueprintCallable, Category = "My HUD")
	float GetHealth();
	UFUNCTION(BlueprintCallable, Category = "My HUD")
	float GetMana();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My HUD")
	float Health = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My HUD")
	float Mana = 0.f;
	
};
