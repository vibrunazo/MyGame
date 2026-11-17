#include "HitReactionEffectComponent.h"
#include "AssCombatComponent.h"
#include "AbilitySystemComponent.h"

void UHitReactionEffectComponent::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	AActor* Target = ActiveGEContainer.Owner ? ActiveGEContainer.Owner->GetAvatarActor() : nullptr;
	if (UAssCombatComponent* Reactor = UAssCombatComponent::Find(Target))
	{
		Reactor->OnHitReaction(Reaction, GESpec);
	}
}
