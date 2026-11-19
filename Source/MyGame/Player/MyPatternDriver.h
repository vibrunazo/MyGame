#pragma once

#include "CoreMinimal.h"
#include "AssPatternDriver.h"
#include "MyPatternDriver.generated.h"

/**
 * The plugin's pattern driver as MyGame's enemies read the world (Ass Step 7; the survey's answers): health from the
 * attribute set, control and move control from the character (the tree's HasControl and HasMoveControl decorators), no
 * stagger of its own (MyGame's hit stun takes control away), the fight on while the target lives, a move's pace as the
 * walk the tree's Walk and NoWalk tasks set, and facing as its FocusPlayer and ResetFocus tasks set it.
 */
UCLASS(ClassGroup = AI)
class MYGAME_API UMyPatternDriver : public UAssPatternDriver
{
	GENERATED_BODY()

public:
	/** The target the pattern fights, or none before the enemy aggroes. */
	ACharacter* GetPatternTarget() const { return GetTarget(); }

protected:
	float GetHealthFraction(const ACharacter* Character) const override;
	bool IsStaggered(const ACharacter* Character) const override { return false; }
	bool CanAct(const ACharacter* Character) const override;
	bool CanMove(const ACharacter* Character) const override;
	bool IsFightOn() const override;
	void SetPace(ACharacter* Self, float Multiplier) override;
	void SetFacing(EAssFacing Facing) override;
};
