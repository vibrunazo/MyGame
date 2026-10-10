// Makes a gameplay effect move or stun its target when applied: hitstun, knockback, launch or camera shake.
// The strength comes from the spec's SetByCaller magnitudes (data.knockback, data.launch.x/y/z, data.camshake),
// which abilities set through their effect containers.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "HitReactionEffectComponent.generated.h"

UENUM(BlueprintType)
enum class EHitReaction : uint8
{
	// Get-hit montage and the stun counter (towards stun immunity); the effect itself grants status.hitstun
	HitStun,
	// Slide away along the attacker's facing, by data.knockback
	Knockback,
	// Launch by (data.launch.x, .y, .z), turned to the attacker's facing, with a short loss of control
	Launch,
	// Shake the camera by data.camshake
	CameraShake,
};

UCLASS(DisplayName = "Hit Reaction")
class MYGAME_API UHitReactionEffectComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Hit Reaction")
	EHitReaction Reaction = EHitReaction::HitStun;

	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
};
