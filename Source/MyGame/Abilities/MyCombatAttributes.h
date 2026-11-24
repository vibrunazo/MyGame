#pragma once

#include "CoreMinimal.h"
#include "AssAttributeSet.h"
#include "MyCombatAttributes.generated.h"

/**
 * MyGame's characters' attributes (Ass Step 11): the plugin's set with MyGame's starting values and its two rules the
 * plugin's set leaves to a game:
 * - a dead character takes no instant change (no damage, no healing, no mana);
 * - the character hears of its damage inside the change that dealt it (IGetHit::OnDamaged, then OnDie when it killed),
 *   as MyGame's own set did before the move.
 * Rot Speed stays in UMyAttributeSet.
 */
UCLASS()
class MYGAME_API UMyCombatAttributes : public UAssAttributeSet
{
	GENERATED_BODY()

public:
	UMyCombatAttributes();
	bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
};
