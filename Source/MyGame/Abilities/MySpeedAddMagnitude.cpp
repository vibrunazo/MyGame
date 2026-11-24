#include "MySpeedAddMagnitude.h"
#include "AssAttributeSet.h"
#include "../MyGameplayTags.h"

UMySpeedAddMagnitude::UMySpeedAddMagnitude()
{
	// the target's Move Speed, snapshot at application: its base value is the walk speed set at BeginPlay
	WalkSpeed = FGameplayEffectAttributeCaptureDefinition(UAssAttributeSet::GetMoveSpeedAttribute(), EGameplayEffectAttributeCaptureSource::Target, true);
	RelevantAttributesToCapture.Add(WalkSpeed);
}

float UMySpeedAddMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	float Base = 0.f;
	if (const FGameplayEffectAttributeCaptureSpec* Captured = Spec.CapturedRelevantAttributes.FindCaptureSpecByDefinition(WalkSpeed, true))
	{
		Captured->AttemptCalculateAttributeBaseValue(Base);
	}
	return Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_Speed, true, 0.f) * Base;
}
