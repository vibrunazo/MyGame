// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_ApplyEffect.h"
#include "IGetHit.h"
#include "EffectEventSettings.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"


void UANS_ApplyEffect::NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration)
{
    UE_LOG(LogTemp, Warning, TEXT("Im a notify beginning"));
    AActor* MyActor = MeshComp->GetOwner();
    IGetHit* MyChar = Cast<IGetHit>(MyActor);
    if (MyChar)
    {
        UAbilitySystemComponent* GAS = MyChar->GetAbilitySystemComponent();
        FGameplayEventData Payload = FGameplayEventData();
        UEffectEventSettings* Settings = NewObject<UEffectEventSettings>(this);
        // UEffectEventSettings Settings = UEffectEventSettings(FObjectInitializer::Get());
        Settings->EffectsToApply = EffectsToApply;
        Payload.OptionalObject = Settings;
        FGameplayTag EffectApplyTag = FGameplayTag::RequestGameplayTag(TEXT("notify.effect.apply"));
        GAS->HandleGameplayEvent(EffectApplyTag, &Payload);
    }
    // ActiveEffects.Add(UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToActor(MyActor, EffectsToApply));
    // ActiveEffects = UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToActor(MyActor, EffectsToApply);
}

void UANS_ApplyEffect::NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation)
{
    UE_LOG(LogTemp, Warning, TEXT("Im a notify ending"));
    AActor* MyActor = MeshComp->GetOwner();
    // UMyBlueprintFunctionLibrary::RemoveEffectsFromActor(MyActor, ActiveEffects.Pop(true));
    // ActiveEffects = {};
    IGetHit* MyChar = Cast<IGetHit>(MyActor);
    if (MyChar)
    {
        UAbilitySystemComponent* GAS = MyChar->GetAbilitySystemComponent();
        FGameplayEventData Payload = FGameplayEventData();
        FGameplayTag EffectRemoveTag = FGameplayTag::RequestGameplayTag(TEXT("notify.effect.remove"));
        GAS->HandleGameplayEvent(EffectRemoveTag, &Payload);
    }

}