// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"
#include "Abilities/IGetHit.h"
#include "MyGameInstance.h"
#include "AbilitySystemComponent.h"
#include "Props/ItemDataAsset.h"

#include "AbilitySystemInterface.h"


/// <summary>
/// Used by Items and self apply notify. Abilities have their own version of this
/// </summary>
/// <param name="Char"></param>
/// <param name="Container"></param>
/// <param name="Item"></param>
/// <returns></returns>
FActiveGameplayEffectHandle UMyBlueprintFunctionLibrary::ApplyEffectContainerToChar(IGetHit* Char, FEffectContainer Container, UItemDataAsset* Item)
{
    UAbilitySystemComponent* GAS = Char->GetAbilitySystemComponent();
    if (!ensure(GAS != nullptr)) return FActiveGameplayEffectHandle();
    if (!Container.EffectClass) return FActiveGameplayEffectHandle();
    FGameplayEffectContextHandle Context = GAS->MakeEffectContext();
    // FGameplayTag
    if (Item && Item->CueTag.ToString() != "")
    {
        GAS->AddGameplayCue(Item->CueTag, Context);
        UE_LOG(LogTemp, Warning, TEXT("Added %s to Context"), *Item->CueTag.ToString());
    }
    FGameplayEffectSpecHandle NewHandle = GAS->MakeOutgoingSpec(Container.EffectClass, 0.f, Context);
    for (auto &&Mag : Container.Magnitudes)
    {
        // NewHandle.Data.Get()->dis;
        NewHandle.Data.Get()->SetSetByCallerMagnitude(Mag.GameplayTag, Mag.Magnitude);
    }
    return *Char->OnGetHitByEffect(NewHandle, nullptr);
    
    // FGameplayEffectSpecHandle NewHandle = MakeOutgoingGameplayEffectSpec(Effect);
    //    NewHandle.Data.Get()->SetSetByCallerMagnitude(HitStunTag, HitStun);
}

void UMyBlueprintFunctionLibrary::RemoveEffectsFromActor(AActor* Actor, TArray<FActiveGameplayEffectHandle> ActiveEffects)
{
    IGetHit* Char = Cast<IGetHit>(Actor);
    if (!Char) return;
    UAbilitySystemComponent* GAS = Char->GetAbilitySystemComponent();
    if (!GAS) return;
    for (auto &&Effect : ActiveEffects)
    {
        GAS->RemoveActiveGameplayEffect(Effect);
    }
    
}
FActiveGameplayEffect UMyBlueprintFunctionLibrary::GetActiveEffectFromHandle(UAbilitySystemComponent* GAS, const FActiveGameplayEffectHandle Handle)
{
    auto we = GAS->GetActiveGameplayEffect(Handle);
    return *we;
}
UGameplayAbility* UMyBlueprintFunctionLibrary::GetAnimatingAbilityFromActor(AActor* OnActor)
{
    if (!OnActor) return nullptr;
    IAbilitySystemInterface* GASInterface = Cast<IAbilitySystemInterface>(OnActor);
    if (!GASInterface) return nullptr;
    return GetAnimatingAbility(GASInterface->GetAbilitySystemComponent());
}
UGameplayAbility* UMyBlueprintFunctionLibrary::GetAnimatingAbility(UAbilitySystemComponent* GAS)
{
    if (!GAS) return nullptr;
    return GAS->GetAnimatingAbility();
}
TArray<FActiveGameplayEffectHandle> UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToActor(AActor* Actor, TArray<FEffectContainer> Containers, UItemDataAsset* Item)
{
    IGetHit* Char = Cast<IGetHit>(Actor);
    if (!Char) return {};
    return ApplyAllEffectContainersToChar(Char, Containers, Item);
}

TArray<FActiveGameplayEffectHandle> UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToChar(IGetHit* Char, TArray<FEffectContainer> Containers, UItemDataAsset* Item)
{
    TArray<FActiveGameplayEffectHandle> result = {};
    for (auto &&Container : Containers)
    {
        FActiveGameplayEffectHandle NewActiveEffect = ApplyEffectContainerToChar(Char, Container, Item);
        result.Add(NewActiveEffect);
    }
    return result;
}
/** Returns const pointer to the actual active gamepay effect structure */
//const FActiveGameplayEffect* GetActiveGameplayEffect(const FActiveGameplayEffectHandle Handle) const;