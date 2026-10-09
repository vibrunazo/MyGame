// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_ApplyEffect.h"
#include "IGetHit.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"


void UANS_ApplyEffect::NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    IGetHit* MyChar = MeshComp ? Cast<IGetHit>(MeshComp->GetOwner()) : nullptr;
    if (!MyChar) return;
    // NotifyEnd removes these; the cap only guarantees they expire if it never comes (death, a skipped section).
    // It's in world time while the montage can be slowed by attack speed and hit pauses, hence the slack.
    const float SafetyCap = TotalDuration + 2.f;
    TArray<FActiveGameplayEffectHandle>& Active = ActiveEffectsByMesh.FindOrAdd(MeshComp);
    Active.Append(UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToChar(MyChar, EffectsToApply, nullptr, SafetyCap));
}

void UANS_ApplyEffect::NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, const FAnimNotifyEventReference& EventReference)
{
    TArray<FActiveGameplayEffectHandle> Active;
    if (!ActiveEffectsByMesh.RemoveAndCopyValue(MeshComp, Active)) return;
    if (MeshComp) UMyBlueprintFunctionLibrary::RemoveEffectsFromActor(MeshComp->GetOwner(), Active);
    // drop entries for meshes that were destroyed mid-notify
    for (auto It = ActiveEffectsByMesh.CreateIterator(); It; ++It)
    {
        if (!It->Key.IsValid()) It.RemoveCurrent();
    }
}
