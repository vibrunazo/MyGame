// Fill out your copyright notice in the Description page of Project Settings.


#include "HitBox.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "../Abilities/GetHit.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"

// Sets default values
AHitBox::AHitBox()
{
	PrimaryActorTick.bCanEverTick = true;

	MyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = MyRoot;
	OnActorBeginOverlap.AddDynamic(this, &AHitBox::OnHitboxBeginOverlap);
	// bGenerateOverlapEvents = true;
	// overlapeve
	// MySphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Collision"));
	// MySphere->SetupAttachment(RootComponent);
	// MySphere->SetSphereRadius

}

// Called when the game starts or when spawned
void AHitBox::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHitBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHitBox::AddComponentsToBones(TArray<FName> Bones)
{
	for (auto &&Bone : Bones)
	{
		USphereComponent* NewSphere = NewObject<USphereComponent>(this);
		// NewSphere->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		NewSphere->RegisterComponent();
		NewSphere->SetSphereRadius(30.0f);
		// NewSphere->bGenerateOverlapEvents = true;
		if (!ensure(GetInstigator() != nullptr)) return;
		if (!ensure(GetOwner() != nullptr)) return;
		USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GetInstigator()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		NewSphere->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
		
	}
	
}

// Instigator is the PLAYER PAWN
// Owner is the Actor who spawned the Hitbox (either player or a projectile)

void AHitBox::OnHitboxBeginOverlap(AActor* OverlappingActor, AActor* OtherActor)
{
	IGetHit *Target = Cast<IGetHit>(OtherActor);
	if (!Target) return;
	if (!ensure(GetInstigator() != nullptr)) return;
	if (GetInstigator() != OtherActor && Target->IsAlive())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s Overlapped %s"), *GetInstigator()->GetName(), *OtherActor->GetName());
		FGameplayTag HitConnectTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.connect"));
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetInstigator(), HitConnectTag, FGameplayEventData());
		ApplyAllEffects(Target);
	}
}

void AHitBox::ApplyAllEffects(class IGetHit* Target)
{
	for (auto &&Effect : EffectsToApply)
	{
		ApplyOneEffect(Effect, Target);
	}
	
}

void AHitBox::ApplyOneEffect(FGameplayEffectSpecHandle Effect, class IGetHit* Target)
{
	// FGameplayEffectSpecHandle *Handle =  FGameplayEffectSpecHandle(Effect);
	Target->OnGetHitByEffect(Effect);
}