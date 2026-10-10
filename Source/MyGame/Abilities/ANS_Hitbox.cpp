// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_Hitbox.h"
#include "../MyGameplayTags.h"
#include "../Player/HitBox.h"
//#include "../Player/HitboxSettings.h"

#include "AbilitySystemBlueprintLibrary.h"

void UANS_Hitbox::NotifyBegin(USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{

	FGameplayTag HitStartTag = MyGameplayTags::Notify_Hit_Start;
	FGameplayEventData Payload = FGameplayEventData();
	//auto NewContainer = new UHitboxesContainer(Hitboxes);
	auto NewContainer = NewObject<UHitboxesContainer>(GetTransientPackage());
	NewContainer->Hitboxes = Hitboxes;
	NewContainer->NumHits = NumHits;
	NewContainer->HitCooldown = HitCooldown;
	Payload.OptionalObject = Cast<UObject>(NewContainer);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), HitStartTag, Payload);

	if (MeshComp->GetOwner() && !Cast<APawn>(MeshComp->GetOwner()))
	{
		AActor* Owner = MeshComp->GetOwner();

		if (!Owner) return;
		FVector Loc = Owner->GetActorLocation();
		FActorSpawnParameters params;
		params.bNoFail = true;
		params.Instigator = (APawn*)(Owner);
		params.Owner = Owner;
		AHitBox* NewHB = Owner->GetWorld()->SpawnActor<AHitBox>(AHitBox::StaticClass(), Loc, FRotator::ZeroRotator, params);
		NewHB->AddComponentsFromContainer(NewContainer);
		NewHB->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale);
		
	}
	
	
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
}

void UANS_Hitbox::NotifyEnd(USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	TArray<USceneComponent*> Children;
	MeshComp->GetChildrenComponents(false, Children);

	for (USceneComponent* Component : Children)
	{
		if (!Component->GetOwner()) continue;
		if (AHitBox* Hitbox = Cast<AHitBox>(Component->GetOwner()))
		{
			Hitbox->Destroy();
		}
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}