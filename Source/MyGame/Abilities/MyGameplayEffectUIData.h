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
	FLinearColor Color;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FSlateBrush Icon;
};

/**
 * 
 */
UCLASS()
class MYGAME_API UMyGameplayEffectUIData : public UGameplayEffectUIData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff UI")
	FBuffUI BuffUI;
};
