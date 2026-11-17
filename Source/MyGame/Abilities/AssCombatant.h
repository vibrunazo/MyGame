#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayEffectTypes.h"
#include "AssCombatant.generated.h"

UINTERFACE(MinimalAPI)
class UAssCombatant : public UInterface
{
	GENERATED_BODY()
};

/**
 * What the combat code asks of the actor it runs on (Ass Plan.md 4.1): the combat part of MyGame's IGetHit, under
 * its final name. The mechanics themselves (stun, knockback, launch, hit pause, control, body block) are
 * UAssCombatComponent's. Blueprints call HasControl, HasMoveControl and IsAlive on the implementing class's own
 * UFUNCTIONs, which override these.
 */
class MYGAME_API IAssCombatant
{
	GENERATED_BODY()

public:
	// Alive: hits, targets and the AI skip the dead
	virtual bool IsAlive() = 0;
	// Its side: a hit never lands between two of one team
	virtual uint8 GetTeam() = 0;
	// Projectiles don't hit it (status.immune.proj)
	virtual bool IsProjectileImmune() = 0;
	// It can act: no hit stun, no lost control, not disabled
	virtual bool HasControl() = 0;
	// It can move: as HasControl, and not rooted (status.nomove)
	virtual bool HasMoveControl() = 0;
	// Applies an effect to it (a hit's payload, an ability's self effects, an item's); the active handle for one with a
	// duration
	virtual FActiveGameplayEffectHandle OnGetHitByEffect(FGameplayEffectSpecHandle Effect) = 0;
	// A hit reaction's camera shake at this strength; the game decides what shakes
	virtual void PlayHitCameraShake(float Strength) = 0;
};
