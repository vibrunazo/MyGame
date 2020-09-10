// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UMyUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="My User Widget")
	class UMyGameInstance* GetMyGameInstance();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="My User Widget")
	TArray<class UItemDataAsset*> GetInventory();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="My User Widget")
	float GetMaxHealthAttr();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="My User Widget")
	float GetAttackAttr();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="My User Widget")
	float GetDefenseAttr();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="My User Widget")
	float GetSpeedAttr();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "My User Widget")
	float GetAttackSpeedAttr();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category="My User Widget")
	void BPOnEnd();
	
};
