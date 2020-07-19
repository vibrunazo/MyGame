// Fill out your copyright notice in the Description page of Project Settings.


#include "CastAbility.h"
#include "ICastProjectile.h"
#include "../Player/MyProjectile.h"
#include "../Player/HitBox.h"
#include "../Player/HitBoxSettings.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

void UCastAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo * ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData * TriggerEventData)
{
    // UE_LOG(LogTemp, Warning, TEXT("Cast ability Activated"));
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    FGameplayTag CastTag = FGameplayTag::RequestGameplayTag(TEXT("notify.projectile.cast"));
    UAbilityTask_WaitGameplayEvent* CastTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, CastTag);
    CastTask->EventReceived.AddDynamic(this, &UCastAbility::OnCast);
    CastTask->ReadyForActivation();
}

void UCastAbility::OnCast(const FGameplayEventData Payload)
{
    // UE_LOG(LogTemp, Warning, TEXT("Cast Projectile here"));
    ICastProjectile* HeWhoCasts = Cast<ICastProjectile>(GetAvatarActorFromActorInfo());
    if (!HeWhoCasts) return;
    FTransform Trans = HeWhoCasts->GetProjectileSpawn();

    FActorSpawnParameters params;
    params.bNoFail = true;
    params.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
    params.Owner = GetAvatarActorFromActorInfo();
    AMyProjectile* NewProj = GetWorld()->SpawnActor<AMyProjectile>(ProjectileToSpawn, Trans.GetLocation(), Trans.GetRotation().Rotator(), params);

    FVector Loc = CurrentActorInfo->AvatarActor->GetActorLocation();
    FActorSpawnParameters hbparams;
    hbparams.bNoFail = true;
    hbparams.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
    hbparams.Owner = NewProj;
    AHitBox* NewHB = GetWorld()->SpawnActor<AHitBox>(HitBoxClass, Loc, FRotator::ZeroRotator, hbparams);
    NewHB->AttachToActor(NewProj, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    NewProj->HitboxRef = NewHB;
    NewHB->HitSound = HitSound;
    NewHB->BlockSound = BlockSound;
    NewHB->HitParticles = HitParticles;
    NewHB->BlockParticles = BlockParticles;
    NewHB->EffectsToApply = MakeSpecHandles();
    FHitboxSettings Settings = FHitboxSettings();
    Settings.SphereRadius = 40.f;
    Settings.bIsSphere = true;
    NewHB->AddComponentsFromSettings(Settings);
    //NewHB->AddHitSphere(40.f);
}