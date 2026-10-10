// A hit landing: the hit (or, on a stun-immune target, block) sound and sparks of the hitbox that landed it.
// Executed by AHitBox on the target as GameplayCue.hit.impact: Location/Normal = impact point and normal,
// SourceObject = the hitbox (its sounds and particles), RawMagnitude = 1 when blocked.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GCN_HitImpact.generated.h"

UCLASS()
class MYGAME_API UGCN_HitImpact : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
