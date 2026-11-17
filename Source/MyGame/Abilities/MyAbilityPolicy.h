#pragma once

#include "CoreMinimal.h"
#include "AssAbilityPolicy.h"
#include "MyAbilityPolicy.generated.h"

/**
 * MyGame's rules for the plugin's abilities (named in DefaultGame.ini's UAssCombatSettings). The play rate is the
 * avatar's AttackSpeed attribute times the ability's MontagesSpeed, 1 for an avatar without attributes (COMBAT.md).
 */
UCLASS()
class MYGAME_API UMyAbilityPolicy : public UObject, public IAssAbilityPolicy
{
	GENERATED_BODY()

public:
	float GetAbilityPlayRate(const UGameplayAbility& Ability, const AActor& Avatar) const override;
};
