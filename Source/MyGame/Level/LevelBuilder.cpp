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
	FTransform RoomLoc = FTransform();
	GenerateWall(RoomLoc, EWallPos::Left);
	for (uint16 i = 0; i < 4; i++)
	{
		ULevelStreaming* NewRoom = GenerateRoom();
		if (NewRoom)
		{
			RoomLoc.SetLocation(FVector(0.0f, 2000.0f * i, 0.0f));
			NewRoom->LevelTransform = RoomLoc;
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
	GenerateWall(RoomLoc, EWallPos::Right);
	
}

ULevelStreaming* ALevelBuilder::GenerateRoom()
{
	// return OnBPCreateLevelByName("Game/Maps/Rooms/Room01");
	ULevelStreaming* NewRoom = OnBPCreateLevelByName(GetRandomRoom()->LevelAddress);
	if (NewRoom)
	{
		NewRoom->SetShouldBeVisible(true);
		NewRoom->SetShouldBeLoaded(true);
	}

	return NewRoom;
}

AStaticMeshActor* ALevelBuilder::GenerateWall(FTransform Where, EWallPos Pos)
{
	FVector Loc = Where.GetLocation();
	FRotator Rot = Where.Rotator();
	switch (Pos)
	{
	case EWallPos::Top:
		Loc.X += 1000.0f; Loc.Z += 100.0f;
		break;

	case EWallPos::Bottom:
		Loc.X -= 1000.0f; Loc.Z += 100.0f;
		break;
	
	case EWallPos::Left:
		Loc.Y -= 1000.0f; Loc.Z += 100.0f;
		Rot.Yaw = 90.0f;
		break;
	
	case EWallPos::Right:
		Loc.Y += 1000.0f; Loc.Z += 100.0f;
		Rot.Yaw = 90.0f;
		break;
	}
	return GenerateWall(FTransform(Rot, Loc));
}

AStaticMeshActor* ALevelBuilder::GenerateWall(FTransform Where)
{
	// return OnBPCreateLevelByName("Game/Maps/Rooms/Room01");
	FVector Loc = Where.GetLocation();
	// Loc.Y += 1000.0f; Loc.Z += 100.0f;
	FActorSpawnParameters params;
	params.bNoFail = true;
	AStaticMeshActor* NewWall = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Loc, Where.Rotator(), params);
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

