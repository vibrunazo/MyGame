// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HitboxSettings.generated.h"


USTRUCT(BlueprintType)
struct FHitboxSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TArray<FName> BoneNames = TArray<FName>();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	float SphereRadius = 40.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FVector BoxExtent = FVector(30.f, 30.f, 30.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FTransform HitboxTransform = FTransform();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	bool bIsSphere = true;
};

/**
 * 
 */
UCLASS(BlueprintType)
class MYGAME_API UHitboxesContainer : public UObject
{
	GENERATED_BODY()

public:
	UHitboxesContainer();
	UHitboxesContainer(TArray<FHitboxSettings> NewHitboxes);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TArray<FHitboxSettings> Hitboxes;
	
};
