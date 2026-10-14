// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_RotateChar.generated.h"

/**
 * Spins the mesh (not the actor) while the notify lasts, RotationSpeed turns per second. The angle follows the
 * animation's time since the notify began, so a spin ends at the same angle whatever the frame rate.
 */
UCLASS()
class MYGAME_API UANS_RotateChar : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	// Turns per second (a yaw of 1 is a full turn each second)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FRotator RotationSpeed;

	// The mesh's rotation (relative to its actor) at Position of Montage, if a spin notify there drives it
	static bool MeshRotationAt(const USkeletalMeshComponent* MeshComp, const class UAnimMontage* Montage, float Position, FQuat& OutRotation);

private:
	// the mesh's rotation in its Blueprint, which the spin starts from and returns to
	static FRotator RestRotation(const USkeletalMeshComponent* MeshComp);
	// the spin RotationSpeed reaches Elapsed seconds into the notify
	FQuat SpinAt(const USkeletalMeshComponent* MeshComp, float Elapsed) const;

};
