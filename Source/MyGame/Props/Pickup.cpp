// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"
#include "ItemDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
// #include "../MyBlueprintFunctionLibrary.h"
#include "../Abilities/IGetHit.h"
#include "AbilitySystemComponent.h"
#include "../Abilities/MyAttributeSet.h"

// Sets default values
APickup::APickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BoxCollision->SetupAttachment(RootComponent);
	BoxCollision->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OnActorBeginOverlap.AddDynamic(this, &APickup::OnPickupBeginOverlap);

}

// Called when the game starts or when spawned
void APickup::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickup::OnPickupBeginOverlap(AActor* OverlappingActor, AActor* OtherActor)
{

	// UE_LOG(LogTemp, Warning, TEXT("%s Overlapped %s"), *OverlappingActor->GetName(), *OtherActor->GetName());
	IGetHit* Char = Cast<IGetHit>(OtherActor);
	if (!Char || Char->GetTeam() != TeamWhoCanPickup) return;
	if (!bMaxHPCanPickup)
	{
		float hp = Char->GetAttributes()->GetHealth();
		float maxhp = Char->GetAttributes()->GetMaxHealth();
		if (hp >= maxhp) return;
	}
	if (ItemData)
	{
		Char->AddItemToInventory(ItemData);
	}
	Destroy();
}
