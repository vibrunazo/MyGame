// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"
#include "Abilities/IGetHit.h"
#include "MyGameInstance.h"
#include "AbilitySystemComponent.h"
#include "Props/ItemDataAsset.h"

void UMyBlueprintFunctionLibrary::ApplyEffectContainerToChar(IGetHit* Char, FEffectContainer Container, UItemDataAsset* Item)
{
    UAbilitySystemComponent* GAS = Char->GetAbilitySystemComponent();
    if (!ensure(GAS != nullptr)) return;
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
    Char->OnGetHitByEffect(NewHandle, nullptr);
    
    // FGameplayEffectSpecHandle NewHandle = MakeOutgoingGameplayEffectSpec(Effect);
    //    NewHandle.Data.Get()->SetSetByCallerMagnitude(HitStunTag, HitStun);
}

void UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToChar(IGetHit* Char, TArray<FEffectContainer> Containers, UItemDataAsset* Item)
{
    for (auto &&Container : Containers)
    {
        ApplyEffectContainerToChar(Char, Container, Item);
    }
}
