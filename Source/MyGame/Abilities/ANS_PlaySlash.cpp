// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_PlaySlash.h"
#include "../Player/MyCharacter.h"

void UANS_PlaySlash::NotifyBegin(USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float TotalDuration)
{
	AMyCharacter* Char = Cast<AMyCharacter>(MeshComp->GetOwner());
	if (Char)
	{
		switch (SlashLocation)
		{
		case ESlash::Punch:
			Template = Char->PunchSlash;
			break;
		case ESlash::Kick:
			Template = Char->KickSlash;
			break;
		default:
			break;
		}
		
	}

	Super::NotifyBegin(MeshComp, Animation, TotalDuration);

}