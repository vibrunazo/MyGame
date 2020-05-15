// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_ApplyEffect.h"
#include "Components/SkeletalMeshComponent.h"


void UANS_ApplyEffect::NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration)
{
    // UE_LOG(LogTemp, Warning, TEXT("Im a notify beginning"));
    AActor* MyActor = MeshComp->GetOwner();
    ActiveEffects = UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToActor(MyActor, EffectsToApply);
}

void UANS_ApplyEffect::NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation)
{
    // UE_LOG(LogTemp, Warning, TEXT("Im a notify ending"));
    AActor* MyActor = MeshComp->GetOwner();
    UMyBlueprintFunctionLibrary::RemoveEffectsFromActor(MyActor, ActiveEffects);

}