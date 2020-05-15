// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "ANS_ApplyEffect.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UANS_ApplyEffect : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	void NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration) override;
	void NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ANS Apply Effect")
	TArray<FEffectContainer> EffectsToApply;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ANS Apply Effect")
	TArray<FActiveGameplayEffectHandle> ActiveEffects;
	
};
