// Fill out your copyright notice in the Description page of Project Settings.


#include "HitBox.h"
#include "HitboxSettings.h"
#include "../Abilities/IGetHit.h"

#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"

// Sets default values
AHitBox::AHitBox()
{
	PrimaryActorTick.bCanEverTick = true;

	MyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = MyRoot;
	//OnActorBeginOverlap.AddDynamic(this, &AHitBox::OnHitboxBeginOverlap);
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


// Instigator is the PLAYER PAWN
// Owner is the Actor who spawned the Hitbox (either player or a projectile)

void AHitBox::HitboxTouched(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp=nullptr , int32 OtherBodyIndex=0, bool bFromSweep=false, const FHitResult& SweepResult=FHitResult())
{
	//UE_LOG(LogTemp, Warning, TEXT("%s touched %s"), *GetInstigator()->GetName(), *Other->GetName());
	//UE_LOG(LogTemp, Warning, TEXT("%s touched %s"), *OverlappedComp->GetName(), *OtherComp->GetFullName());
	//UE_LOG(LogTemp, Warning, TEXT("component Z is %f"), OverlappedComp->GetRelativeLocation().Z);
	if (!GetWorld()) return;
	float CurTime = GetWorld()->GetTimeSeconds();
	IGetHit* Target = nullptr;
	Target = Cast<IGetHit>(Other);
	IGetHit* Source = Cast<IGetHit>(GetInstigator());
	if (!Target || !Source) return;
	if (!ensure(GetInstigator() != nullptr)) return;
	if (HitboxChannel == EHitboxChannel::Projectile && Target->IsProjectileImmune())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot hit target because it's immune to projectiles"));
		return;
	}
	FEnemyHitState CurEnemyState = ActorsHit.FindOrAdd(Other);
	uint8 CurHitCount = CurEnemyState.NumHits;
	float LastHit = CurEnemyState.LastHitTime;
	//uint8 CurHitCount = ActorsHit.FindOrAdd(Other);
	bool bHaveIHitThisGuyTheMaxNumOfTimes = CurHitCount >= NumHits;
	bool bHasEnoughTimePassedSinceLastHit = (CurTime - LastHit) > HitCooldown;
	UE_LOG(LogTemp, Warning, TEXT("I hit someone, CurHitCount: %d, NumHits: %d"), CurHitCount, NumHits);
	if (GetInstigator() != Other && Target->IsAlive() && !bHaveIHitThisGuyTheMaxNumOfTimes && bHasEnoughTimePassedSinceLastHit && Source->GetTeam() != Target->GetTeam())
	{
		FHitResult OutHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());
		Params.AddIgnoredActor(GetInstigator());
		bool DidIHit = GetWorld()->LineTraceSingleByChannel
		(
			OutHit,
			OverlappedComp->GetComponentLocation(),
			Other->GetActorLocation(),
			ECollisionChannel::ECC_Visibility,
			Params,
			FCollisionResponseParams()
		);
		//DrawDebugLine(GetWorld(), OverlappedComp->GetComponentLocation(), Other->GetActorLocation(), FColor::Green, false, 1.f, 0.f, 2.f);
		if (DidIHit)
		{
			//DrawDebugPoint(GetWorld(), OutHit.ImpactPoint, 10.f, FColor::Red, false, 2.f);
			//UE_LOG(LogTemp, Warning, TEXT("hit? %d, %s"), DidIHit, *OutHit.ToString());
			//UE_LOG(LogTemp, Warning, TEXT("sweep? %d, %s"), bFromSweep, *SweepResult.ToString());
			FRotator SparkRot = FRotator::ZeroRotator;
			SparkRot.Yaw = OutHit.ImpactNormal.Rotation().Yaw;
			// UE_LOG(LogTemp, Warning, TEXT("%s Overlapped %s"), *GetInstigator()->GetName(), *OtherActor->GetName());
			CurEnemyState.NumHits = CurHitCount + 1;
			CurEnemyState.LastHitTime = CurTime;
			ActorsHit.Emplace(Other, CurEnemyState);
			//ActorsHit.Emplace(Other, CurHitCount + 1);
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
		else UE_LOG(LogTemp, Warning, TEXT("Hittrace failed"));
	}
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
	NumHits = Hitboxes->NumHits;
	HitCooldown = Hitboxes->HitCooldown;
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
		AddOneComponentFromBone(Settings, Bone);
	}
	if (Bones.Num() == 0)
	{
		AddOneComponentFromBone(Settings, NAME_None);
	}
}

void AHitBox::AddOneComponentFromBone(FHitboxSettings Settings, FName Bone)
{
	UPrimitiveComponent* NewHitComponent;
	if (Settings.bIsSphere)	NewHitComponent = AddHitSphere(Settings.SphereRadius);
	else NewHitComponent = AddHitBoxComponent(Settings.BoxExtent);
	HitComponents.Add(NewHitComponent);
	if (Bone != NAME_None)
	{
		USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GetInstigator()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		NewHitComponent->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
	}
	NewHitComponent->SetRelativeTransform(Settings.HitboxTransform);
	//NewHitComponent->SetWorldScale3D(FVector(1.f, 1.f, 0.1f));
	NewHitComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &AHitBox::HitboxTouched);
	TArray<AActor*> OutOverlappingActors;
	NewHitComponent->GetOverlappingActors(OutOverlappingActors);
	if (OutOverlappingActors.Num() > 0)
	{
		for (auto&& TouchedActor : OutOverlappingActors)
		{
			HitboxTouched(NewHitComponent, TouchedActor);
		}
	}
}

USphereComponent* AHitBox::AddHitSphere(float SphereRadius)
{
	USphereComponent* NewComp = NewObject<USphereComponent>(this);
	NewComp->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	NewComp->RegisterComponent();
	NewComp->SetSphereRadius(SphereRadius);
	NewComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	NewComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	return NewComp;
}

UBoxComponent* AHitBox::AddHitBoxComponent(FVector BoxExtent)
{
	UBoxComponent* NewComp = NewObject<UBoxComponent>(this);
	NewComp->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	NewComp->RegisterComponent();
	NewComp->SetBoxExtent(BoxExtent);
	NewComp->OnComponentBeginOverlap.AddUniqueDynamic(this, &AHitBox::HitboxTouched);
	return NewComp;
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