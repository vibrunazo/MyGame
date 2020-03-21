// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "HitBox.h"

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
		NewSphere->SetSphereRadius(50.0f);
		// NewSphere->bGenerateOverlapEvents = true;
		if (!ensure(GetInstigator() != nullptr)) return;
		if (!ensure(GetOwner() != nullptr)) return;
		USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GetInstigator()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		NewSphere->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Bone);
		
	}
	
}

void AHitBox::OnHitboxBeginOverlap(AActor* OverlappingActor, AActor* OtherActor)
{
	UE_LOG(LogTemp, Warning, TEXT("Overlapped: %s"), *OtherActor->GetName());
}