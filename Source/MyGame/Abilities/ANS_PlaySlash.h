// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_TimedNiagaraEffect.h"
#include "ANS_PlaySlash.generated.h"

UENUM(BlueprintType)
enum class ESlash : uint8
{
	Punch,
	Kick,
};

/**
 * 
 */
UCLASS()
class MYGAME_API UANS_PlaySlash : public UAnimNotifyState_TimedNiagaraEffect
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float TotalDuration) override;

	UPROPERTY(EditAnywhere, Category = NiagaraSystem, meta = (ToolTip = "The location to spawn the Slash Trail Effect"))
	ESlash SlashLocation;
	
};
