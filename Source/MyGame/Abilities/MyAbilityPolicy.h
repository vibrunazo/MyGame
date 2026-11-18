#pragma once

#include "CoreMinimal.h"
#include "AssAbilityPolicy.h"
#include "MyAbilityPolicy.generated.h"

/**
 * MyGame's rules for the plugin's abilities (named in DefaultGame.ini's UAssCombatSettings).
 * - The play rate is the avatar's AttackSpeed attribute times the ability's speed, 1 for an avatar without attributes
 *   (COMBAT.md).
 * - A hit is today's effect specs (Ass Plan.md Step 3): PrepareHit makes one per part when the hit is prepared (a window
 *   opening, a cast), so it captures the attacker's values then; each Apply hands its part's spec to the target. The
 *   typed parts' effects are the game instance's (HitDamageEffect and so on): Damage sets data.damage to the negated
 *   magnitude, Stun Ass.Data.HitStun to its time, Push Ass.Data.Knockback to its X, Launch Ass.Data.Launch.X, .Y and .Z;
 *   an Effect part is its own class and magnitudes. Heal and Poise aren't MyGame's.
 */
UCLASS()
class MYGAME_API UMyAbilityPolicy : public UObject, public IAssAbilityPolicy
{
	GENERATED_BODY()

public:
	float GetAbilityPlayRate(const UGameplayAbility& Ability, const AActor& Avatar) const override;
	void PrepareHit(FAssPreparedHit& Hit, const UGameplayAbility& Ability) const override;
	void ApplyDamage(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const override;
	void ApplyHeal(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const override;
	void ApplyPoise(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const override;
	void ApplyStun(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const override;
	void ApplyPush(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const override;
	void ApplyLaunch(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const override;
	void ApplyEffect(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const override;

private:
	// The part's spec goes to the target, as the hitbox applied each spec before Step 3
	static void ApplySpec(const FAssPreparedPart& Part, AActor& Target);
};
