#include "MyAbilityPolicy.h"
#include "IGetHit.h"
#include "MyAttributeSet.h"
#include "../MyGameInstance.h"
#include "../MyGameplayTags.h"
#include "AssAbility.h"
#include "AssCombatant.h"
#include "AssHitPath.h"
#include "AssHitResult.h"
#include "AssTags.h"

#include "GameMapsSettings.h"
#include "GameplayEffect.h"

namespace
{
	// The game instance class's defaults: the hit effects are set on BP_MyGI
	const UMyGameInstance* GameInstanceDefaults()
	{
		const UClass* GameInstanceClass = GetDefault<UGameMapsSettings>()->GameInstanceClass.TryLoadClass<UMyGameInstance>();
		return GameInstanceClass ? GameInstanceClass->GetDefaultObject<UMyGameInstance>() : nullptr;
	}

	FGameplayEffectSpecHandle MakeSpec(const UGameplayAbility& Ability, TSubclassOf<UGameplayEffect> Class, const FAssPreparedHit& Hit)
	{
		return Class ? Ability.MakeOutgoingGameplayEffectSpec(Class, (float)Hit.Level) : FGameplayEffectSpecHandle();
	}
}

float UMyAbilityPolicy::GetAbilityPlayRate(const UGameplayAbility& Ability, const AActor& Avatar) const
{
	// reading the attributes changes nothing, but IGetHit's getter isn't const
	IGetHit* Me = Cast<IGetHit>(const_cast<AActor*>(&Avatar));
	const UMyAttributeSet* Attributes = Me ? Me->GetAttributes() : nullptr;
	if (!Attributes) return 1.0f;
	const UAssAbility* AssAbility = Cast<UAssAbility>(&Ability);
	return Attributes->GetAttackSpeed() * (AssAbility ? AssAbility->MontagesSpeed : 1.0f);
}

void UMyAbilityPolicy::PrepareHit(FAssPreparedHit& Hit, const UGameplayAbility& Ability) const
{
	const UMyGameInstance* Defaults = GameInstanceDefaults();
	if (!Defaults) return;
	const FAssHitPayload& Payload = Hit.Payload;
	for (FAssPreparedPart& Part : Hit.Parts)
	{
		switch (Part.Kind)
		{
		case EAssPayloadPart::Damage:
			// MyGame's damage effect takes the health change: a negative number
			Part.Spec = MakeSpec(Ability, Defaults->HitDamageEffect, Hit);
			if (Part.Spec.IsValid()) Part.Spec.Data->SetSetByCallerMagnitude(MyGameplayTags::Data_Damage, -Payload.Damage.Magnitude);
			break;
		case EAssPayloadPart::Stun:
			Part.Spec = MakeSpec(Ability, Defaults->HitStunEffect, Hit);
			if (Part.Spec.IsValid()) Part.Spec.Data->SetSetByCallerMagnitude(AssTags::Data_HitStun, Payload.Stun.Time);
			break;
		case EAssPayloadPart::Push:
			// the knockback slides along the attacker's facing (x of the attacker's frame)
			Part.Spec = MakeSpec(Ability, Defaults->HitKnockbackEffect, Hit);
			if (Part.Spec.IsValid()) Part.Spec.Data->SetSetByCallerMagnitude(AssTags::Data_Knockback, Payload.Push.Vector.X);
			break;
		case EAssPayloadPart::Launch:
			// X, Y and Z in that order, as the abilities' lists set them
			Part.Spec = MakeSpec(Ability, Defaults->HitLaunchEffect, Hit);
			if (Part.Spec.IsValid())
			{
				Part.Spec.Data->SetSetByCallerMagnitude(AssTags::Data_Launch_X, Payload.Launch.Velocity.X);
				Part.Spec.Data->SetSetByCallerMagnitude(AssTags::Data_Launch_Y, Payload.Launch.Velocity.Y);
				Part.Spec.Data->SetSetByCallerMagnitude(AssTags::Data_Launch_Z, Payload.Launch.Velocity.Z);
			}
			break;
		case EAssPayloadPart::Effect:
		{
			const FAssEffectEntry& Entry = Payload.Effects[Part.EffectIndex];
			Part.Spec = MakeSpec(Ability, Entry.EffectClass.LoadSynchronous(), Hit);
			if (!Part.Spec.IsValid()) break;
			for (const FAssMagnitudePair& Magnitude : Entry.Magnitudes)
			{
				Part.Spec.Data->SetSetByCallerMagnitude(Magnitude.GameplayTag, Magnitude.Magnitude);
			}
			break;
		}
		default:
			break;
		}
	}
}

void UMyAbilityPolicy::ApplySpec(const FAssPreparedPart& Part, AActor& Target)
{
	IAssCombatant* Combatant = Cast<IAssCombatant>(&Target);
	if (!Combatant || !IsValid(&Target) || !Part.Spec.IsValid()) return;
	Combatant->OnGetHitByEffect(Part.Spec);
}

void UMyAbilityPolicy::ApplyDamage(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const
{
	ApplySpec(Part, Target);
	Result.Inputs.Magnitude = Hit.Payload.Damage.Magnitude;
}

void UMyAbilityPolicy::ApplyHeal(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const
{
	ApplySpec(Part, Target);
}

void UMyAbilityPolicy::ApplyPoise(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const
{
	ApplySpec(Part, Target);
}

void UMyAbilityPolicy::ApplyStun(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const
{
	ApplySpec(Part, Target);
	Result.StunTime = Hit.Payload.Stun.Time;
}

void UMyAbilityPolicy::ApplyPush(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const
{
	ApplySpec(Part, Target);
	Result.bPushed = Part.Spec.IsValid();
}

void UMyAbilityPolicy::ApplyLaunch(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const
{
	ApplySpec(Part, Target);
	Result.bLaunched = Part.Spec.IsValid();
}

void UMyAbilityPolicy::ApplyEffect(const FAssPreparedHit& Hit, const FAssPreparedPart& Part, AActor& Target, FAssHitResult& Result) const
{
	ApplySpec(Part, Target);
}
