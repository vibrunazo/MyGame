// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_RotateChar.h"

void UANS_RotateChar::NotifyTick(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	if (MeshComp->IsSimulatingPhysics()) return;
	FRotator CurrentRotation = RotationSpeed * FrameDeltaTime * 360.f;
	FRotator LastRotation = MeshComp->GetRelativeRotation();
	//UE_LOG(LogTemp, Warning, TEXT("Notify Ticking, last rot: %s"), *LastRotation.ToString());
	MeshComp->AddLocalRotation(CurrentRotation);
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);
}


void UANS_RotateChar::NotifyEnd(USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation)
{
	if (MeshComp->IsSimulatingPhysics()) return;
	MeshComp->SetRelativeRotation(FRotator(0.f, 270.f, 0.f));
	FRotator LastRotation = MeshComp->GetRelativeRotation();
	//UE_LOG(LogTemp, Warning, TEXT("Notify Ending, last rot: %s"), *LastRotation.ToString());
	Super::NotifyEnd(MeshComp, Animation);
}