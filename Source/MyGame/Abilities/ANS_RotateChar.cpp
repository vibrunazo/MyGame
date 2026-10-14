// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_RotateChar.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

void UANS_RotateChar::NotifyTick(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp->IsSimulatingPhysics()) return;
	// how long the notify has run in the montage's time (a frame that starts or ends mid-notify counts only its part)
	const UAnimInstance* Anim = MeshComp->GetAnimInstance();
	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	const FAnimNotifyEvent* Event = EventReference.GetNotify();
	if (!Anim || !Montage || !Event) return;
	const float Elapsed = FMath::Clamp(Anim->Montage_GetPosition(Montage) - Event->GetTriggerTime(), 0.f, Event->GetDuration());
	MeshComp->SetRelativeRotation(SpinAt(MeshComp, Elapsed));
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
}


void UANS_RotateChar::NotifyEnd(USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp->IsSimulatingPhysics()) return;
	MeshComp->SetRelativeRotation(RestRotation(MeshComp));
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FRotator UANS_RotateChar::RestRotation(const USkeletalMeshComponent* MeshComp)
{
	const USkeletalMeshComponent* Template = Cast<USkeletalMeshComponent>(MeshComp->GetArchetype());
	return Template ? Template->GetRelativeRotation() : FRotator(0.f, 270.f, 0.f);
}

FQuat UANS_RotateChar::SpinAt(const USkeletalMeshComponent* MeshComp, float Elapsed) const
{
	return RestRotation(MeshComp).Quaternion() * (RotationSpeed * Elapsed * 360.f).Quaternion();
}

bool UANS_RotateChar::MeshRotationAt(const USkeletalMeshComponent* MeshComp, const UAnimMontage* Montage, float Position, FQuat& OutRotation)
{
	if (!MeshComp || !Montage) return false;
	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		const UANS_RotateChar* Spin = Cast<UANS_RotateChar>(Event.NotifyStateClass);
		if (!Spin || Position < Event.GetTriggerTime() || Position > Event.GetEndTriggerTime()) continue;
		OutRotation = Spin->SpinAt(MeshComp, Position - Event.GetTriggerTime());
		return true;
	}
	return false;
}
