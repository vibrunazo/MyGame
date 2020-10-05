// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Player/HitboxSettings.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_Hitbox.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UANS_Hitbox : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TArray<FHitboxSettings> Hitboxes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "How many times I can hit the same Actor?"))
	uint8 NumHits = 1;
	
};
