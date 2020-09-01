// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "AN_PlaySoundFromChar.generated.h"

UENUM(BlueprintType)
enum class EWooshType : uint8
{
	PunchWeak,
	KickWeak,
	PunchStrong,
	KickStrong,
};

/**
 * 
 */
UCLASS()
class MYGAME_API UAN_PlaySoundFromChar : public UAnimNotify_PlaySound
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (ExposeOnSpawn = true, ToolTip = "The type of Woosh sound to make. Will get the actual sound file from the Character. Actual sound file might change depending on current buffs."))
	EWooshType WooshType;
	
};
