// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"
#include "Abilities/IGetHit.h"
#include "MyGameInstance.h"
#include "AbilitySystemComponent.h"

void UMyBlueprintFunctionLibrary::ApplyEffectContainerToChar(IGetHit* Char, FEffectContainer Container)
{
    UAbilitySystemComponent* GAS = Char->GetAbilitySystemComponent();
    if (!ensure(GAS != nullptr)) return;
    FGameplayEffectSpecHandle NewHandle = GAS->MakeOutgoingSpec(Container.EffectClass, 0.f, GAS->MakeEffectContext());
    for (auto &&Mag : Container.Magnitudes)
    {
        NewHandle.Data.Get()->SetSetByCallerMagnitude(Mag.GameplayTag, Mag.Magnitude);
    }
    Char->OnGetHitByEffect(NewHandle, nullptr);
    
    // FGameplayEffectSpecHandle NewHandle = MakeOutgoingGameplayEffectSpec(Effect);
    //    NewHandle.Data.Get()->SetSetByCallerMagnitude(HitStunTag, HitStun);
}

void UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToChar(IGetHit* Char, TArray<FEffectContainer> Containers)
{
    for (auto &&Container : Containers)
    {
        ApplyEffectContainerToChar(Char, Container);
    }
}
