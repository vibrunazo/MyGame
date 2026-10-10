#include "InputTriggerDirectionalDoubleTap.h"

ETriggerState UInputTriggerDirectionalDoubleTap::UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime)
{
	Time += DeltaTime;
	const FVector2D Direction = ModifiedValue.Get<FVector2D>();
	if (Direction.Size() < TapDepth)
	{
		LastReleaseTime = Time;
		return ETriggerState::None;
	}
	// a tap starts when the input comes back after being released; holding a direction is not tapping
	const bool bNewTap = LastReleaseTime > LastTapTime;
	bool bDoubleTap = false;
	if (bNewTap && Time - LastTapTime < TapWindow)
	{
		const float Cos = FVector2D::DotProduct(Direction.GetSafeNormal(), LastTapDirection.GetSafeNormal());
		bDoubleTap = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Cos, -1.f, 1.f))) <= MaxAngle;
	}
	if (bNewTap)
	{
		LastTapDirection = Direction;
		LastTapTime = Time;
	}
	return bDoubleTap ? ETriggerState::Triggered : ETriggerState::None;
}
