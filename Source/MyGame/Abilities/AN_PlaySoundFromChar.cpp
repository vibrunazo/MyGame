// Fill out your copyright notice in the Description page of Project Settings.


#include "AN_PlaySoundFromChar.h"

#include "../Player/MyCharacter.h"

void UAN_PlaySoundFromChar::Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation)
{
	AMyCharacter* Char = Cast<AMyCharacter>(MeshComp->GetOwner());
	if (Char)
	{
		switch (WooshType)
		{
		case EWooshType::PunchWeak:
			Sound = Char->PunchWeakWoosh;
			break;
		case EWooshType::KickWeak:
			break;
		case EWooshType::PunchStrong:
			Sound = Char->PunchStrongWoosh;
			break;
		case EWooshType::KickStrong:
			break;
		default:
			break;
		}
	}
	Super::Notify(MeshComp, Animation);
}