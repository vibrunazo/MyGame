#include "MyCombatAttributes.h"
#include "IGetHit.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Pawn.h"

UMyCombatAttributes::UMyCombatAttributes()
{
	// MyGame's starting values: a character's own Max Health, Physical Damage and Defense are set on it at BeginPlay
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMana(40.f);
	InitMaxMana(100.f);
	InitPhysicalDamage(1.f);
	InitDefense(1.f);
	InitAttackSpeed(1.f);
}

bool UMyCombatAttributes::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	// the dead take no instant change
	return GetHealth() > 0.f && Super::PreGameplayEffectExecute(Data);
}

void UMyCombatAttributes::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	const bool bDamage = Data.EvaluatedData.Attribute == GetIncomingDamageAttribute();
	const float Damage = bDamage ? GetIncomingDamage() : 0.f;
	Super::PostGameplayEffectExecute(Data);
	if (!bDamage || Damage <= 0.f) return;

	// The source is who attacked, not what touched: the hit path makes a projectile the effect causer (pushes follow its
	// facing), and the damaged would then aggro on the projectile. The causer only when no attacker is known
	const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	const UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
	AActor* SourceActor = Source && Source->AbilityActorInfo.IsValid() ? Source->AbilityActorInfo->AvatarActor.Get() : nullptr;
	if (!SourceActor) SourceActor = Context.GetEffectCauser();
	AActor* TargetActor = Data.Target.AbilityActorInfo.IsValid() ? Data.Target.AbilityActorInfo->AvatarActor.Get() : nullptr;
	if (IGetHit* HeWhoGetsHit = Cast<IGetHit>(TargetActor))
	{
		HeWhoGetsHit->OnDamaged(SourceActor, -Damage, Data.EffectSpec);
		if (GetHealth() <= 0.f) HeWhoGetsHit->OnDie();
	}
}
