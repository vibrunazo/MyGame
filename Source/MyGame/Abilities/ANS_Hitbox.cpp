// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_Hitbox.h"
//#include "../Player/HitboxSettings.h"

#include "AbilitySystemBlueprintLibrary.h"

void UANS_Hitbox::NotifyBegin(USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float TotalDuration)
{
	UE_LOG(LogTemp, Warning, TEXT("hitbox ans notify"));

	FGameplayTag HitStartTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.start"));
	FGameplayEventData Payload = FGameplayEventData();
	//auto NewContainer = new UHitboxesContainer(Hitboxes);
	auto NewContainer = NewObject<UHitboxesContainer>(this);
	NewContainer->Hitboxes = Hitboxes;
	Payload.OptionalObject = Cast<UObject>(NewContainer);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), HitStartTag, Payload);
	
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);
}