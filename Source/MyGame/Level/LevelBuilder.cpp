// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBuilder.h"
#include "Components/BillboardComponent.h"
#include "Engine/LevelStreaming.h"
#include "AssetRegistryModule.h"
#include "RoomDataAsset.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"


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
			FTransform RoomLoc = FTransform();
			RoomLoc.SetLocation(FVector(0.0f, 2000.0f * i, 0.0f));
			NewRoom->LevelTransform = RoomLoc;
			NewRoom->SetShouldBeVisible(true);
			NewRoom->SetShouldBeLoaded(true);
			// FVector Loc = RoomLoc.GetLocation(); Loc.Y += 1000.0f; Loc.Z += 100.0f;
			// AStaticMeshActor* NewWall = GenerateWall(RoomLoc);
			AStaticMeshActor* NewWall1 = GenerateWall(RoomLoc, EWallPos::Top);
			AStaticMeshActor* NewWall2 = GenerateWall(RoomLoc, EWallPos::Bottom);
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

AStaticMeshActor* ALevelBuilder::GenerateWall(FTransform Where, EWallPos Pos)
{
	FVector Loc;
	switch (Pos)
	{
	case EWallPos::Top:
		Loc = Where.GetLocation(); Loc.X += 1000.0f; Loc.Z += 100.0f;
		break;

	case EWallPos::Bottom:
		Loc = Where.GetLocation(); Loc.X -= 1000.0f; Loc.Z += 100.0f;
		break;
	
	default:
		Loc = Where.GetLocation();
	}
	return GenerateWall(FTransform(Where.GetRotation().Rotator(), Loc));
}

AStaticMeshActor* ALevelBuilder::GenerateWall(FTransform Where)
{
	// return OnBPCreateLevelByName("Game/Maps/Rooms/Room01");
	FVector Loc = Where.GetLocation();
	// Loc.Y += 1000.0f; Loc.Z += 100.0f;
	FActorSpawnParameters params;
	params.bNoFail = true;
	AStaticMeshActor* NewWall = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Loc, FRotator::ZeroRotator, params);
	NewWall->GetStaticMeshComponent()->SetStaticMesh(WallMesh);
	return NewWall;
}

void ALevelBuilder::SetAssetListFromRegistry()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> OutAssets;
	const UClass* RoomClass = URoomDataAsset::StaticClass();
	AssetRegistryModule.Get().GetAssetsByClass(RoomClass->GetFName(), OutAssets);
	AssetDataList = OutAssets;
	// UE_LOG(LogTemp, Warning, TEXT("Found %d"), AssetDataList.Num());
	for (auto &&AssetData : AssetDataList)
	{
		URoomDataAsset* NewRoom = Cast<URoomDataAsset>(AssetData.GetAsset());
		if (NewRoom)
		{
			RoomList.Add(NewRoom);
			// UE_LOG(LogTemp, Warning, TEXT("Found Room %s"), *NewRoom->LevelAddress.ToString());
		}
	}
}


URoomDataAsset* ALevelBuilder::GetRandomRoom()
{
	if (AssetDataList.Num() == 0) return nullptr;
	int32 Index = FMath::RandRange(0, AssetDataList.Num() - 1);
	return RoomList[Index];
}

void ALevelBuilder::SetupExternalReferences()
{
	

}

