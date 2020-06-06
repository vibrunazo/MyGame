// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"
#include "ItemDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "../Abilities/IGetHit.h"
#include "../Abilities/MyAttributeSet.h"
#include "../UI/WidgetActor.h"
#include "../Abilities/LootComponent.h"

#include "AbilitySystemComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
// #include "Components/TimelineComponent.h"

// Sets default values
APickup::APickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// PrimaryActorTick.bCanEverTick = true;

	// RootComponent = RootComp;
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BoxCollision->SetupAttachment(RootComponent);
	BoxCollision->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	// BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	BoxCollision->SetSimulatePhysics(true);
	RootComponent = BoxCollision;
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComp->SetupAttachment(RootComponent);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComp);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Mesh->SetVisibility(true);
	Mesh->SetHiddenInGame(true);
	BoxTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	BoxTrigger->SetupAttachment(RootComponent);
	BoxTrigger->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	BoxTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OnActorBeginOverlap.AddDynamic(this, &APickup::OnPickupBeginOverlap);
	// Blueprint'/Game/UI/BP_WidgetActor.BP_WidgetActor'
	// Blueprint'/Game/UI/BP_UMGActor.BP_UMGActor'
	static ConstructorHelpers::FClassFinder<AWidgetActor> WABPClass(TEXT("/Game/UI/BP_UMGActor"));
	WidgetActorClass = WABPClass.Class;

	LootComponent = CreateDefaultSubobject<ULootComponent>(TEXT("Loot Component"));

}

void APickup::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);
	ApplyItemData();
}

// Called when the game starts or when spawned
void APickup::BeginPlay()
{
	Super::BeginPlay();

	if (bUseRandomPool && LootComponent)
	{
		SetItemData(LootComponent->GetRandomItem());
	}

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &APickup::OnDelayedSpawn, FMath::RandRange(0.05f, 0.3f), false);
	// if I have an owner it means I was dropped by an enemy instead of by being placed in the map in the editor
	if (GetOwner())
	{
		AddActorLocalOffset(FVector(0.f, 0.f, 50.f));
	}
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
		// The UI Widget that shows in the world letting player know he picked up this item
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
	if (PickupSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), PickupSound, GetActorLocation());
	if (PickupParticles) UNiagaraFunctionLibrary::SpawnSystemAttached(PickupParticles, OtherActor->GetRootComponent(), NAME_None, FVector(0.f, 0.f, -80.f), OtherActor->GetActorRotation(), EAttachLocation::SnapToTarget, true);
	// if (PickupParticles) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PickupParticles, GetActorLocation(), FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None);
	Destroy();
}

void APickup::OnDelayedSpawn()
{
	// BoxCollision->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	BoxCollision->SetSimulatePhysics(true);
	if (GetOwner())
	{
		BoxCollision->AddImpulse(FVector(0.f, 0.5, 500.0f), NAME_None, true);
		FVector SpawnLoc = FVector(GetActorLocation());
		SpawnLoc.Z -= 20.f;
		if (SpawnParticles) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SpawnParticles, SpawnLoc, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None);
		if (SpawnSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpawnSound, GetActorLocation());

		if (CurveScale)
		{
			FOnTimelineFloat TimelineCallback;
			FOnTimelineEventStatic TimelineFinishedCallback;
			TimelineCallback.BindUFunction(this, FName("OnTimelineCallback"));
			MyTimeline.AddInterpFloat(CurveScale, TimelineCallback);
			MyTimeline.PlayFromStart();
			MyTimeline.TickTimeline(0.02f);
			GetWorldTimerManager().SetTimer(TimelineTimer, this, &APickup::OnTimelineUpdate, 0.02f, true);
		}
	}
	Mesh->SetHiddenInGame(false);
	FTimerHandle Handle2;
	GetWorldTimerManager().SetTimer(Handle2, this, &APickup::EnablePickup, PickupTime, false);

}

void APickup::OnTimelineCallback()
{
	MyTimeline.GetTimelineLength();
	float Cur = CurveScale->GetFloatValue(MyTimeline.GetPlaybackPosition());
	FVector NewScale = FVector(Cur, Cur, Cur);
	// UE_LOG(LogTemp, Warning, TEXT("timeline callback %f"), Cur);
	RootComp->SetRelativeScale3D(NewScale);
}
void APickup::OnTimelineUpdate()
{
	MyTimeline.TickTimeline(0.02f);

	// UE_LOG(LogTemp, Warning, TEXT("timeline update, %f"), MyTimeline.GetPlaybackPosition());
	if (MyTimeline.GetPlaybackPosition() >= MyTimeline.GetTimelineLength())
	{
		// UE_LOG(LogTemp, Warning, TEXT("stopping timeline"));
		GetWorldTimerManager().ClearTimer(TimelineTimer);
	}
}

void APickup::EnablePickup()
{
	BoxTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// Mesh->SetVisibility(true);
}

void APickup::ApplyItemData()
{
	if (!ItemData) return;
	if (ItemData->PickupMesh) Mesh->SetStaticMesh(ItemData->PickupMesh);
	if (ItemData->SpawnParticles) SpawnParticles = ItemData->SpawnParticles;
	if (ItemData->SpawnSound) SpawnSound = ItemData->SpawnSound;
	if (ItemData->PickupSound) PickupSound = ItemData->PickupSound;
	if (ItemData->PickupParticles) PickupParticles = ItemData->PickupParticles;
	bMaxHPCanPickup = ItemData->bMaxHPCanPickup;
}

void APickup::SetItemData(UItemDataAsset* NewItemData)
{
	if (!NewItemData) return;
	ItemData = NewItemData;
	ApplyItemData();
}