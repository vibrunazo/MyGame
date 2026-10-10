// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HitboxSettings.generated.h"


/**
* Holds the settings for one of the Shape components (Box Component or Sphere Component) of the Hitbox Actor.
* The Hitbox Actor will have an Array of this, because each Hitbox Actor can have multiple components.
* Each Hitbox Component can be attached to a bone. If multiple bones are set, then multiple identical components are created created, one for each bone.
*/
USTRUCT(BlueprintType)
struct FHitboxSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "The socket or bone to attach the Hitbox", AnimNotifyBoneName = "true"))
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
