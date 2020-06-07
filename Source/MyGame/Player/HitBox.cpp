// Fill out your copyright notice in the Description page of Project Settings.


#include "HitBox.h"
#include "../Abilities/IGetHit.h"
#include "HitboxSettings.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
	
	if (GetOwner())	{
		// UE_LOG(LogTemp, Warning, TEXT("I am hitbox and %s is my daddy"), *GetOwner()->GetName());
		GetOwner()->OnDestroyed.AddDynamic(this, &AHitBox::OnOwnerDestroyed);
	}
	// else {UE_LOG(LogTemp, Warning, TEXT("I am hitbox and I has no daddy"));}
}

void AHitBox::OnOwnerDestroyed(AActor* DestroyedActor)
{
	Destroy();
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
		USphereComponent* NewSphere = AddHitSphere();
		// NewSphere->bGenerateOverlapEvents = true;
		if (!ensure(GetInstigator() != nullptr)) return;
		// if (!ensure(GetOwner() != nullptr)) return;
		USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GetInstigator()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		NewSphere->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
	}
}

void AHitBox::AddComponentsFromSettings(TArray<UHitboxSettings*> Settings)
{
	Hitboxes.Append(Settings);
	for (auto&& Setting : Settings)
	{
		AddComponentsFromSetting(Setting);
	}
}

void AHitBox::AddComponentsFromSetting(UHitboxSettings* Setting)
{
	TArray<FName> Bones = Setting->BoneNames;
	for (auto&& Bone : Bones)
	{
		USphereComponent* NewSphere = AddHitSphere();
		if (!ensure(GetInstigator() != nullptr)) return;
		USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GetInstigator()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		NewSphere->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
	}
}

USphereComponent* AHitBox::AddHitSphere()
{
	USphereComponent* NewSphere = NewObject<USphereComponent>(this);
	NewSphere->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	NewSphere->RegisterComponent();
	NewSphere->SetSphereRadius(SphereRadius);
	return NewSphere;
}

// Instigator is the PLAYER PAWN
// Owner is the Actor who spawned the Hitbox (either player or a projectile)

void AHitBox::OnHitboxBeginOverlap(AActor* OverlappingActor, AActor* OtherActor)
{
	IGetHit *Target = nullptr;
	Target = Cast<IGetHit>(OtherActor);
	IGetHit *Source = Cast<IGetHit>(GetInstigator());
	if (!Target || ! Source) return;
	if (!ensure(GetInstigator() != nullptr)) return;
	if (GetInstigator() != OtherActor && Target->IsAlive() && !ActorsHit.Contains(OtherActor) && Source->GetTeam() != Target->GetTeam())
	{
		// UE_LOG(LogTemp, Warning, TEXT("%s Overlapped %s"), *GetInstigator()->GetName(), *OtherActor->GetName());
		ActorsHit.Add(OtherActor);
		FGameplayTag HitConnectTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.connect"));
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetInstigator(), HitConnectTag, FGameplayEventData());
		ApplyAllEffects(Target);
	}
}

void AHitBox::ApplyAllEffects(class IGetHit* Target)
{
	if (!Target) return;
	for (auto &&Effect : EffectsToApply)
	{
		ApplyOneEffect(Effect, Target);
	}
	
}

void AHitBox::ApplyOneEffect(FGameplayEffectSpecHandle Effect, class IGetHit* Target)
{
	if (!Target || !Target->IsValidLowLevel() || !IsValid((UObject*)Target) ) return;
	Target->OnGetHitByEffect(Effect, GetOwner());  // Invalid object index
}