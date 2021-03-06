// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Abilities/MyGameplayEffectUIData.h"

#include "GameplayEffect.h"
#include "Blueprint/UserWidget.h"
#include "MyHealthBar.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UMyHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void SetHealth(float NewHealth);
	UFUNCTION(BlueprintImplementableEvent)
	void SetMaxHealth(float NewMaxHealth);
	UFUNCTION(BlueprintCallable)
	void GetRatioFromHealth(float Health);
	UFUNCTION(BlueprintCallable)
	float GetHealth();
	UFUNCTION(BlueprintImplementableEvent)
	UUserWidget* BP_AddNewDurationBar(float Duration, class UMyGameplayEffectUIData* UIData);
	UFUNCTION(BlueprintImplementableEvent)
	void BP_ResetDurationBar(float Duration, UUserWidget* BarRef);
	void AddDurationToHealthBar(float Duration, const FGameplayEffectSpec& EffectSpec, struct FActiveGameplayEffectHandle ActiveEffectHandle);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="HealthBar")
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar")
	TMap<struct FActiveGameplayEffectHandle, UUserWidget*> MapOfBars;
	
};
