#include "HitReactionEffectComponent.h"
#include "IGetHit.h"
#include "AbilitySystemComponent.h"

void UHitReactionEffectComponent::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	AActor* Target = ActiveGEContainer.Owner ? ActiveGEContainer.Owner->GetAvatarActor() : nullptr;
	if (IGetHit* Reactor = Cast<IGetHit>(Target))
	{
		Reactor->OnHitReaction(Reaction, GESpec);
	}
}
