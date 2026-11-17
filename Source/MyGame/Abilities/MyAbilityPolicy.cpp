#include "MyAbilityPolicy.h"
#include "IGetHit.h"
#include "MyAttributeSet.h"
#include "AssAbility.h"

float UMyAbilityPolicy::GetAbilityPlayRate(const UGameplayAbility& Ability, const AActor& Avatar) const
{
	// reading the attributes changes nothing, but IGetHit's getter isn't const
	IGetHit* Me = Cast<IGetHit>(const_cast<AActor*>(&Avatar));
	const UMyAttributeSet* Attributes = Me ? Me->GetAttributes() : nullptr;
	if (!Attributes) return 1.0f;
	const UAssAbility* AssAbility = Cast<UAssAbility>(&Ability);
	return Attributes->GetAttackSpeed() * (AssAbility ? AssAbility->MontagesSpeed : 1.0f);
}
