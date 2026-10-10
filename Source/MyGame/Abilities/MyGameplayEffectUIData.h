// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectUIData.h"

#include "MyGameplayEffectUIData.generated.h"

USTRUCT(BlueprintType)
struct FBuffUI
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FSlateBrush Icon;
};

// Points a gameplay effect at its UBuffUIDataAsset (the UI's single source for that buff)
UCLASS()
class MYGAME_API UMyGameplayEffectUIData : public UGameplayEffectUIData
{
	GENERATED_BODY()
public:
	// The buff's UI: icon, name, color, description
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff UI")
	TObjectPtr<class UBuffUIDataAsset> BuffUIAsset;

	// The asset's values as the struct the widgets read (empty when no asset is set)
	UFUNCTION(BlueprintPure, Category = "Buff UI")
	FBuffUI GetBuffUI() const;
};
