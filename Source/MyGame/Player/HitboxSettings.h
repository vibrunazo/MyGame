// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HitboxSettings.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class MYGAME_API UHitboxSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TArray<FName> BoneNames;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	float SphereRadius = 40.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FTransform HitboxTransform = FTransform();
	
};
