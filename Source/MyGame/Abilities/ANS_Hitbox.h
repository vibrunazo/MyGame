// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Player/HitboxSettings.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_Hitbox.generated.h"


/**
 * A Hitbox Notify State. Used by the Anim Montage to set when in the animation the Hitbox is created and destroyed.
 * Sends itself as the notify.hit.start and notify.hit.end events' Optional Object (the animation as Optional Object 2);
 * the ability's UAbilityTask_HitboxWindows reads these settings to shape the hitbox it spawns.
 */
UCLASS()
class MYGAME_API UANS_Hitbox : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	// This notify's window in Animation's time (false if Animation doesn't hold it)
	bool GetWindow(const class UAnimSequenceBase* Animation, float& OutStart, float& OutEnd) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TArray<FHitboxSettings> Hitboxes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "How many times I can hit the same Actor?"))
	uint8 NumHits = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "Cooldown between hits allowed against the same Actor."))
	float HitCooldown = 0.1f;
	
};
