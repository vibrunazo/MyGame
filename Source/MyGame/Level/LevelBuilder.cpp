// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBuilder.h"
#include "Components/BillboardComponent.h"
#include "Engine/LevelStreaming.h"
#include "AssetRegistryModule.h"
#include "RoomDataAsset.h"

// Sets default values
ALevelBuilder::ALevelBuilder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BBComp = CreateDefaultSubobject<UBillboardComponent>(TEXT("Dino"));
	RootComponent = BBComp;
	// BBComp->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void ALevelBuilder::BeginPlay()
{
	Super::BeginPlay();
	// ULevelStreaming* NewLevel = OnBPCreateLevelByName(NewRoom->LevelAddress);

	SetAssetListFromRegistry();
	GenerateLevels();
	// GenerateRoom();
}

void ALevelBuilder::GenerateLevels()
{
	for (uint16 i = 0; i < 4; i++)
	{
		ULevelStreaming* NewRoom = GenerateRoom();
		if (NewRoom)
		{
			UE_LOG(LogTemp, Warning, TEXT("Room is valid"));
			FTransform RoomLoc = FTransform();
			RoomLoc.SetLocation(FVector(0.0f, 2000.0f * i, 0.0f));
			NewRoom->LevelTransform = RoomLoc;
			NewRoom->SetShouldBeVisible(true);
			NewRoom->SetShouldBeLoaded(true);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Room failed"));
		}
		
	}
	
}

ULevelStreaming* ALevelBuilder::GenerateRoom()
{
	// return OnBPCreateLevelByName("Game/Maps/Rooms/Room01");
	return OnBPCreateLevelByName(GetRandomRoom()->LevelAddress);
}

void ALevelBuilder::SetAssetListFromRegistry()
{
	UE_LOG(LogTemp, Warning, TEXT("Looking for assets"));
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> OutAssets;
	const UClass* RoomClass = URoomDataAsset::StaticClass();
	AssetRegistryModule.Get().GetAssetsByClass(RoomClass->GetFName(), OutAssets);
	AssetDataList = OutAssets;
	UE_LOG(LogTemp, Warning, TEXT("Found %d"), AssetDataList.Num());
	for (auto &&AssetData : AssetDataList)
	{
		URoomDataAsset* NewRoom = Cast<URoomDataAsset>(AssetData.GetAsset());
		if (NewRoom)
		{
			RoomList.Add(NewRoom);
			UE_LOG(LogTemp, Warning, TEXT("Found Room %s"), *NewRoom->LevelAddress.ToString());
		}
	}
}


URoomDataAsset* ALevelBuilder::GetRandomRoom()
{
	if (AssetDataList.Num() == 0) return nullptr;
	int32 Index = FMath::RandRange(0, AssetDataList.Num() - 1);
	return RoomList[Index];
}

