// Fires once when a 2D input is tapped twice in (about) the same direction: the dash.

#pragma once

#include "CoreMinimal.h"
#include "InputTriggers.h"
#include "InputTriggerDirectionalDoubleTap.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Directional Double Tap"))
class MYGAME_API UInputTriggerDirectionalDoubleTap : public UInputTrigger
{
	GENERATED_BODY()

public:
	// Most seconds between the starts of the two taps
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Settings")
	float TapWindow = 0.6f;

	// How far the stick must go (0..1) to count as a tap; keys always reach 1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Settings")
	float TapDepth = 0.6f;

	// Most degrees between the two taps' directions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Settings")
	float MaxAngle = 20.f;

protected:
	virtual ETriggerType GetTriggerType_Implementation() const override { return ETriggerType::Explicit; }
	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime) override;

private:
	float Time = 0.f;
	float LastTapTime = -990.f;
	float LastReleaseTime = -990.f;
	FVector2D LastTapDirection = FVector2D::ZeroVector;
};
