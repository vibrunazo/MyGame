// Fill out your copyright notice in the Description page of Project Settings.


#include "CastAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ICastProjectile.h"
#include "../Player/MyProjectile.h"

void UCastAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo * ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData * TriggerEventData)
{
    UE_LOG(LogTemp, Warning, TEXT("Cast ability Activated"));
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    FGameplayTag CastTag = FGameplayTag::RequestGameplayTag(TEXT("notify.projectile.cast"));
    UAbilityTask_WaitGameplayEvent* CastTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, CastTag);
    CastTask->EventReceived.AddDynamic(this, &UCastAbility::OnCast);
    CastTask->ReadyForActivation();
}

void UCastAbility::OnCast(const FGameplayEventData Payload)
{
    UE_LOG(LogTemp, Warning, TEXT("Cast Projectile here"));
    ICastProjectile* HeWhoCasts = Cast<ICastProjectile>(GetAvatarActorFromActorInfo());
    if (!HeWhoCasts) return;
    FTransform Trans = HeWhoCasts->GetProjectileSpawn();

    FActorSpawnParameters params;
    params.bNoFail = true;
    params.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
    params.Owner = GetAvatarActorFromActorInfo();
    AMyProjectile* NewActor = GetWorld()->SpawnActor<AMyProjectile>(ProjectileToSpawn, Trans.GetLocation(), Trans.GetRotation().Rotator(), params);
}