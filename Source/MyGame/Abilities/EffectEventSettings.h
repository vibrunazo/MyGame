// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "UObject/NoExportTypes.h"
#include "EffectEventSettings.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UEffectEventSettings : public UObject
{
	GENERATED_BODY()

public:
	// UEffectEventSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TArray<FEffectContainer> EffectsToApply = {};
	
};
