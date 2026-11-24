#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MySpeedAddMagnitude.generated.h"

/**
 * A speed change of data.speed, a share of the walk speed, as an add to Move Speed (Ass Step 11): data.speed x the
 * target's Move Speed base value (its walk speed), taken when the effect applies. MyGame's speed was a multiplier,
 * (1 + the adds) x (1 + the multipliers' offsets); with the adds on Move Speed's base this way and the multipliers kept
 * multiply-additive, Move Speed is that multiplier x the walk speed exactly, however many are active. A set-by-caller
 * amount can't be scaled by an attribute in data, hence this calculation.
 */
UCLASS()
class MYGAME_API UMySpeedAddMagnitude : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMySpeedAddMagnitude();
	float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition WalkSpeed;
};
