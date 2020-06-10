// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_Hitbox.h"
#include "../Player/HitBox.h"
//#include "../Player/HitboxSettings.h"

#include "AbilitySystemBlueprintLibrary.h"

void UANS_Hitbox::NotifyBegin(USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation, float TotalDuration)
{

	FGameplayTag HitStartTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.start"));
	FGameplayEventData Payload = FGameplayEventData();
	//auto NewContainer = new UHitboxesContainer(Hitboxes);
	auto NewContainer = NewObject<UHitboxesContainer>(this);
	NewContainer->Hitboxes = Hitboxes;
	Payload.OptionalObject = Cast<UObject>(NewContainer);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), HitStartTag, Payload);

	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("I'm in editor"));
		AActor* Owner = MeshComp->GetOwner();

		if (!Owner) return;
		FVector Loc = Owner->GetActorLocation();
		UE_LOG(LogTemp, Warning, TEXT("Owner is: %s, is pawn: %d"), *Owner->GetName(), Cast<APawn>(Owner));
		FActorSpawnParameters params;
		params.bNoFail = true;
		params.Instigator = (APawn*)(Owner);
		params.Owner = Owner;
		AHitBox* NewHB = Owner->GetWorld()->SpawnActor<AHitBox>(AHitBox::StaticClass(), Loc, FRotator::ZeroRotator, params);
		NewHB->AddComponentsFromContainer(NewContainer);
		NewHB->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale);
	}
	
	
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);
}

void UANS_Hitbox::NotifyEnd(USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation)
{
	UE_LOG(LogTemp, Warning, TEXT("ans hitbox notify end"));

	TArray<USceneComponent*> Children;
	MeshComp->GetChildrenComponents(false, Children);

	for (USceneComponent* Component : Children)
	{
		
		UE_LOG(LogTemp, Warning, TEXT("Found child %s of %s"), *Component->GetName(), *Component->GetOwner()->GetName());
		if (AHitBox* Hitbox = Cast<AHitBox>(Component->GetOwner()))
		{
			UE_LOG(LogTemp, Warning, TEXT("is hitbox"));
			Hitbox->Destroy();
		}
	}

	Super::NotifyEnd(MeshComp, Animation);
}