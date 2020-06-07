// Fill out your copyright notice in the Description page of Project Settings.


#include "AN_Hitstart.h"

void UAN_Hitstart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	UE_LOG(LogTemp, Warning, TEXT("AN HitStart Notify"));

	Super::Notify(MeshComp, Animation);
}