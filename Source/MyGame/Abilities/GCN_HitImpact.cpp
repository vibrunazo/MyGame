#include "GCN_HitImpact.h"
#include "../Player/HitBox.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogHitCue, Log, All);

bool UGCN_HitImpact::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const AHitBox* Hitbox = Cast<AHitBox>(Parameters.SourceObject.Get());
	if (!Hitbox) return false;
	const bool bBlocked = Parameters.RawMagnitude > 0.f;
	// blocks fall back to the hit sound and sparks when the hitbox has no block ones
	USoundBase* Sound = bBlocked && Hitbox->BlockSound ? Hitbox->BlockSound : Hitbox->HitSound;
	UNiagaraSystem* Particles = bBlocked && Hitbox->BlockParticles ? Hitbox->BlockParticles : Hitbox->HitParticles;
	if (Sound) UGameplayStatics::PlaySoundAtLocation(Hitbox, Sound, Hitbox->GetActorLocation());
	if (Particles) UNiagaraFunctionLibrary::SpawnSystemAtLocation(Hitbox, Particles, Parameters.Location, FRotator(0.f, Parameters.Normal.Rotation().Yaw, 0.f));
	UE_LOG(LogHitCue, Verbose, TEXT("HitImpactCue on %s blocked=%d"), *GetNameSafe(MyTarget), bBlocked);
	return true;
}
