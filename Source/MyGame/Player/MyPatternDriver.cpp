#include "MyPatternDriver.h"
#include "MyCharacter.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "../Abilities/MyAttributeSet.h"

namespace
{
	AMyCharacter* Mine(const ACharacter* Character)
	{
		return const_cast<AMyCharacter*>(Cast<AMyCharacter>(Character));
	}
}

float UMyPatternDriver::GetHealthFraction(const ACharacter* Character) const
{
	AMyCharacter* Char = Mine(Character);
	const UAbilitySystemComponent* Abilities = Char ? Char->GetAbilitySystemComponent() : nullptr;
	if (!Abilities) return 1.f;
	const float Max = Abilities->GetNumericAttribute(UMyAttributeSet::GetMaxHealthAttribute());
	return Max > 0.f ? Abilities->GetNumericAttribute(UMyAttributeSet::GetHealthAttribute()) / Max : 1.f;
}

bool UMyPatternDriver::CanAct(const ACharacter* Character) const
{
	AMyCharacter* Char = Mine(Character);
	return !Char || Char->HasControl();
}

bool UMyPatternDriver::CanMove(const ACharacter* Character) const
{
	AMyCharacter* Char = Mine(Character);
	return !Char || Char->HasMoveControl();
}

bool UMyPatternDriver::IsFightOn() const
{
	// The tree idles with no target, and forgets a dead one
	const ACharacter* Target = GetTarget();
	return Target && !IsDown(Target);
}

void UMyPatternDriver::SetPace(ACharacter* Self, float Multiplier)
{
	// The tree's Walk (the passive branch's walk to its spot) and NoWalk (the attack branch): a walk is any pace under 1
	if (AMyCharacter* Char = Mine(Self)) Char->SetWalking(Multiplier < 1.f);
}

void UMyPatternDriver::SetFacing(EAssFacing Facing)
{
	AAIController* Controller = GetController();
	ACharacter* Self = GetSelf();
	if (!Controller || !Self) return;
	if (Facing == EAssFacing::Target)
	{
		// FocusPlayer: the controller looks at the target and the character turns with it
		Controller->SetFocus(GetTarget());
		Self->bUseControllerRotationYaw = true;
		return;
	}
	// ResetFocus: no focus, the character turns as it moves (the tree also clears it while control is lost, as each pass
	// restarts through the attack branch's first tasks)
	Controller->ClearFocus(EAIFocusPriority::Gameplay);
	Self->bUseControllerRotationYaw = false;
}
