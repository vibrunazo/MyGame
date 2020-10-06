// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Player/HitboxSettings.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_Hitbox.generated.h"


/**
 * A Hitbox Notify State. Used by the Anim Montage to set when in the animation the Hitbox is created and destroyed.
 * Contains all the settings that will be stored on an UHitboxesContainer and passed as an Optional Object through an event
 * that will be captured by the Gameplay Ability that will use it to create the Hitbox and initialize it.
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "Cooldown between hits allowed against the same Actor."))
	float HitCooldown = 0.1f;
	
};
