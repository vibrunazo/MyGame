// Makes a gameplay effect move or stun its target when applied: hitstun, knockback, launch or camera shake.
// The strength comes from the spec's SetByCaller magnitudes (Ass.Data.Knockback, Ass.Data.Launch.X/y/z, Ass.Data.CamShake),
// which abilities set through their effect containers.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "HitReactionEffectComponent.generated.h"

UENUM(BlueprintType)
enum class EHitReaction : uint8
{
	// Get-hit montage and the stun counter (towards stun immunity); the effect itself grants Ass.Status.HitStun
	HitStun,
	// Slide away along the attacker's facing, by Ass.Data.Knockback
	Knockback,
	// Launch by (Ass.Data.Launch.X, .y, .z), turned to the attacker's facing, with a short loss of control
	Launch,
	// Shake the camera by Ass.Data.CamShake
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
