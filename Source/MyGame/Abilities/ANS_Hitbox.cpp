// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_Hitbox.h"
#include "../MyGameplayTags.h"
#include "../Player/HitBox.h"
//#include "../Player/HitboxSettings.h"

#include "AbilitySystemBlueprintLibrary.h"

void UANS_Hitbox::NotifyBegin(USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{

	// the notify itself is the payload: an ability's UAbilityTask_HitboxWindows reads its shapes and limits
	FGameplayEventData Payload;
	Payload.OptionalObject = this;
	Payload.OptionalObject2 = Animation;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), MyGameplayTags::Notify_Hit_Start, Payload);

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
		NewHB->SetupHitboxes(Hitboxes, NumHits, HitCooldown);
		NewHB->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale);
		
	}
	
	
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
}

void UANS_Hitbox::NotifyEnd(USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	// the window closes: the ability's hitbox task tests the rest of the window, then destroys its hitbox
	FGameplayEventData Payload;
	Payload.OptionalObject = this;
	Payload.OptionalObject2 = Animation;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), MyGameplayTags::Notify_Hit_End, Payload);

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

bool UANS_Hitbox::GetWindow(const UAnimSequenceBase* Animation, float& OutStart, float& OutEnd) const
{
	if (!Animation) return false;
	for (const FAnimNotifyEvent& Event : Animation->Notifies)
	{
		if (Event.NotifyStateClass != this) continue;
		OutStart = Event.GetTriggerTime();
		OutEnd = Event.GetEndTriggerTime();
		return true;
	}
	return false;
}
