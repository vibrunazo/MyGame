// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"
#include "ItemDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
// #include "../MyBlueprintFunctionLibrary.h"
#include "../Abilities/IGetHit.h"
#include "AbilitySystemComponent.h"
#include "../Abilities/MyAttributeSet.h"
#include "../UI/WidgetActor.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

// Sets default values
APickup::APickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// PrimaryActorTick.bCanEverTick = true;

	// RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	// RootComponent = RootComp;
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BoxCollision->SetupAttachment(RootComponent);
	BoxCollision->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	BoxCollision->SetSimulatePhysics(true);
	RootComponent = BoxCollision;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	// Mesh->SetVisibility(false);
	BoxTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	BoxTrigger->SetupAttachment(RootComponent);
	BoxTrigger->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	BoxTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OnActorBeginOverlap.AddDynamic(this, &APickup::OnPickupBeginOverlap);
	// Blueprint'/Game/UI/BP_WidgetActor.BP_WidgetActor'
	// Blueprint'/Game/UI/BP_UMGActor.BP_UMGActor'
	static ConstructorHelpers::FClassFinder<AWidgetActor> WABPClass(TEXT("/Game/UI/BP_UMGActor"));
	WidgetActorClass = WABPClass.Class;

}

// Called when the game starts or when spawned
void APickup::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &APickup::EnablePickup, 1.0f, false);
	
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
		if (WidgetActorClass)
		{
			FVector Loc = GetActorLocation();
			FActorSpawnParameters params;
			params.Owner = this;
			AWidgetActor* NewWidget = GetWorld()->SpawnActor<AWidgetActor>(WidgetActorClass, Loc, FRotator::ZeroRotator, params);
			if (NewWidget)
			{
				NewWidget->SetWidgetText(ItemData);
			}
		}
	}
	Destroy();
}

void APickup::EnablePickup()
{
	BoxTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// Mesh->SetVisibility(true);
}