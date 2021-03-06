// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "IGetHit.h"

UMyAttributeSet::UMyAttributeSet()
    : Health(100.0f)
    , MaxHealth(100.0f)
    , Mana(40.0f)
    , MaxMana(100.0f)
    , Attack(1.0f)
    , Defense(1.0f)
    , Speed(1.0f)
    , AttackSpeed(1.0f)
    , RotSpeed(1.0f)
{
}

bool UMyAttributeSet::PreGameplayEffectExecute(struct FGameplayEffectModCallbackData & Data)
{
    if (GetHealth() <= 0)
    {
        return false;
    }
    return true;
}

void UMyAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
    // Get the Target actor, which should be our owner
	AActor* TargetActor = nullptr;
    FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
    UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
    // Get the Source actor
    AActor* SourceActor = nullptr;
    AController* SourceController = nullptr;
    if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
    {
        SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
        SourceController = Source->AbilityActorInfo->PlayerController.Get();
        if (SourceController == nullptr && SourceActor != nullptr)
        {
            if (APawn* Pawn = Cast<APawn>(SourceActor))
            {
                SourceController = Pawn->GetController();
            }
        }

        // Set the causer actor based on context if it's set
        if (Context.GetEffectCauser())
        {
            SourceActor = Context.GetEffectCauser();
        }
    }


	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
	}

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
        // Handle other health changes such as from healing or direct modifiers
		// First clamp it
        float DamageValue = Data.EvaluatedData.Magnitude;
        /*FGameplayTagContainer tags = FGameplayTagContainer();
        Data.EffectSpec.GetAllAssetTags(tags);
        UE_LOG(LogTemp, Warning, TEXT("damage effect, tags: %s"), *tags.ToString());*/
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
        IGetHit* HeWhoGetsHit = Cast<IGetHit>(TargetActor);
        if (HeWhoGetsHit && DamageValue < 0)
        {
            HeWhoGetsHit->OnDamaged(SourceActor, DamageValue, Data.EffectSpec);
            if (GetHealth() == 0)
            {
                HeWhoGetsHit->OnDie();
            }
        }
    }

    if (Data.EvaluatedData.Attribute == GetManaAttribute())
    {
        SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
    }
    // if (Data.EvaluatedData.Attribute == GetSpeedAttribute())
    // {
    //     UE_LOG(LogTemp, Warning, TEXT("attribute detected speed change"));
    //     IGetHit* HeWhoGetsHit = Cast<IGetHit>(TargetActor);
    //     HeWhoGetsHit->OnSpeedChange();
    // }
}