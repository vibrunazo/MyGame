// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayEffectTypes.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "ANS_ApplyEffect.generated.h"

/**
 * Applies EffectsToApply to the animated character for the length of the notify state and removes exactly those
 * effects when it ends. Effects never outlast the notify, even when their data asks for a negative ("forever") duration.
 */
UCLASS()
class MYGAME_API UANS_ApplyEffect : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	void NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	void NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ANS Apply Effect")
	TArray<FEffectContainer> EffectsToApply;

private:
	// Notify objects live in the anim asset and are shared by every character playing it, so track effects per mesh
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TArray<FActiveGameplayEffectHandle>> ActiveEffectsByMesh;
};
