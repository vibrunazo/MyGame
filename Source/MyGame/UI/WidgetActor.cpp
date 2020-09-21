// Fill out your copyright notice in the Description page of Project Settings.


#include "WidgetActor.h"
#include "Components/WidgetComponent.h"
#include "PickupTextWidget.h"
#include "../Props/ItemDataAsset.h"
#include "Components/SceneComponent.h"

// Sets default values
AWidgetActor::AWidgetActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	MyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = MyRoot;
	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	WidgetComp->SetupAttachment(RootComponent);
	WidgetComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);

}

// Called when the game starts or when spawned
void AWidgetActor::BeginPlay()
{
	Super::BeginPlay();


	
}

void AWidgetActor::SetWidgetText(UItemDataAsset* NewItem)
{
	if (WidgetComp)
	{
		UUserWidget* UW = WidgetComp->GetUserWidgetObject();
		UPickupTextWidget* PuW = Cast<UPickupTextWidget>(UW);
		if (PuW)
		{
			PuW->SetWidgetText(NewItem);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Widget Comp on widget actor"));
	}
}