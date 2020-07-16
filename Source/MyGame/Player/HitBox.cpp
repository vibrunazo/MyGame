// Fill out your copyright notice in the Description page of Project Settings.


#include "HitBox.h"
#include "HitboxSettings.h"
#include "../Abilities/IGetHit.h"

#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"

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

//void AHitBox::AddComponentsToBones(TArray<FName> Bones)
//{
//	for (auto &&Bone : Bones)
//	{
//		USphereComponent* NewSphere = AddHitSphere();
//		// NewSphere->bGenerateOverlapEvents = true;
//		if (!ensure(GetInstigator() != nullptr)) return;
//		// if (!ensure(GetOwner() != nullptr)) return;
//		USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GetInstigator()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
//		NewSphere->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
//	}
//}

void AHitBox::AddComponentsFromContainer(UHitboxesContainer* Container)
{
	if (!Container) return;
	//Hitboxes.Append(Container->Hitboxes);
	Hitboxes = Container;
	for (auto&& Settings : Container->Hitboxes)
	{
		AddComponentsFromSettings(Settings);
	}
}

void AHitBox::AddComponentsFromSettings(FHitboxSettings Settings)
{
	if (!ensure(GetInstigator() != nullptr)) return;
	TArray<FName> Bones = Settings.BoneNames;
	for (auto&& Bone : Bones)
	{
		USceneComponent* NewHitComponent;
		if (Settings.bIsSphere)	NewHitComponent = AddHitSphere(Settings.SphereRadius);
		else NewHitComponent = AddHitBoxComponent(Settings.BoxExtent);
		USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GetInstigator()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		NewHitComponent->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
		NewHitComponent->SetRelativeTransform(Settings.HitboxTransform);
		//NewHitComponent->SetWorldScale3D(FVector(1.f, 1.f, 0.1f));
	}
}

USphereComponent* AHitBox::AddHitSphere(float SphereRadius)
{
	USphereComponent* NewSphere = NewObject<USphereComponent>(this);
	NewSphere->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	NewSphere->RegisterComponent();
	NewSphere->SetSphereRadius(SphereRadius);
	return NewSphere;
}

UBoxComponent* AHitBox::AddHitBoxComponent(FVector BoxExtent)
{
	UBoxComponent* NewBox = NewObject<UBoxComponent>(this);
	NewBox->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	NewBox->RegisterComponent();
	NewBox->SetBoxExtent(BoxExtent);
	return NewBox;
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
	if (GetInstigator() != OtherActor && Target->IsAlive() && !ActorsHit.Contains(OtherActor) && Source->GetTeam() != Target->GetTeam() && GetWorld())
	{
		FHitResult OutHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());
		Params.AddIgnoredActor(GetInstigator());
		bool DidIHit = GetWorld()->LineTraceSingleByChannel
		(
			OutHit,
			GetActorLocation(),
			OtherActor->GetActorLocation(),
			ECollisionChannel::ECC_Visibility,
			Params,
			FCollisionResponseParams()
		);
		if (DidIHit)
		{
			UE_LOG(LogTemp, Warning, TEXT("hit? %d, %s"), DidIHit, *OutHit.ToString());
			FRotator SparkRot = FRotator::ZeroRotator;
			SparkRot.Yaw = OutHit.ImpactNormal.Rotation().Yaw;
			// UE_LOG(LogTemp, Warning, TEXT("%s Overlapped %s"), *GetInstigator()->GetName(), *OtherActor->GetName());
			ActorsHit.Add(OtherActor);
			FGameplayTag HitConnectTag = FGameplayTag::RequestGameplayTag(TEXT("notify.hit.connect"));
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetInstigator(), HitConnectTag, FGameplayEventData());
			ApplyAllEffects(Target);
			UAbilitySystemComponent* GAS = Target->GetAbilitySystemComponent();
			FGameplayTag HitStunImmuneTag = FGameplayTag::RequestGameplayTag(TEXT("status.stunimmune"));
			if (GAS && GAS->HasMatchingGameplayTag(HitStunImmuneTag))
			{
				if (BlockSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), BlockSound, GetActorLocation());
				else if (HitSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, GetActorLocation());
				if (BlockParticles) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BlockParticles, OutHit.ImpactPoint, SparkRot);
				else if (HitParticles) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitParticles, OutHit.ImpactPoint, SparkRot);
			}
			else
			{
				if (HitSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, GetActorLocation());
				if (HitParticles) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitParticles, OutHit.ImpactPoint, SparkRot);
			}
		}
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
	//if (!Target || !Target->IsValidLowLevel() || !IsValid((UObject*)Target)) return;
	if (!Target) return;
	if (!Target->IsValidLowLevel()) return;
	Target->OnGetHitByEffect(Effect, GetOwner());
}