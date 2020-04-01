// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBuilder.h"
#include "Components/BillboardComponent.h"
#include "Engine/LevelStreaming.h"
#include "AssetRegistryModule.h"
#include "RoomDataAsset.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"


TArray<EWallPos> ALLDIRECTIONS = {EWallPos::Left, EWallPos::Right, EWallPos::Bottom, EWallPos::Top};
int8 GetXFromDir(EWallPos Dir)
{
	if (Dir == EWallPos::Top) return 1;
	if (Dir == EWallPos::Bottom) return -1;
	return 0;
}
int8 GetYFromDir(EWallPos Dir)
{
	if (Dir == EWallPos::Right) return 1;
	if (Dir == EWallPos::Left) return -1;
	return 0;
}


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
	BuildGrid();
	GenerateLevels();
	// GenerateRoom();
}

void ALevelBuilder::GenerateLevels()
{
	FTransform RoomLoc = FTransform();
	for (auto &&Tile : Grid)
	{
		ULevelStreaming* NewRoom = GenerateRoom(Tile.Key, Tile.Value.RoomType);
		UE_LOG(LogTemp, Warning, TEXT("created %s room at %s"), *Tile.Value.RoomType->LevelAddress.ToString(), *Tile.Key.ToString());
		BuildWalls(Tile);
	}
	
	for (auto &&Wall : AllWalls)
	{
		UE_LOG(LogTemp, Warning, TEXT("Wall: %s"), *Wall.Key);
	}
	
}

ULevelStreaming* ALevelBuilder::GenerateRandomRoom(FTransform Where)
{
	// return OnBPCreateLevelByName("Game/Maps/Rooms/Room01");
	URoomDataAsset* NewRoomType = GetRandomRoom();
	ULevelStreaming* NewRoom = OnBPCreateLevelByName(NewRoomType->LevelAddress);
	if (NewRoom)
	{
		NewRoom->LevelTransform = Where;
		NewRoom->SetShouldBeVisible(true);
		NewRoom->SetShouldBeLoaded(true);
	}
	return NewRoom;
}

ULevelStreaming* ALevelBuilder::GenerateRoom(FCoord Where, class URoomDataAsset* RoomType)
{
	ULevelStreaming* NewRoom = OnBPCreateLevelByName(RoomType->LevelAddress);
	if (NewRoom)
	{
		NewRoom->LevelTransform.SetLocation(GetLocFromGrid(Where));
		NewRoom->SetShouldBeVisible(true);
		NewRoom->SetShouldBeLoaded(true);
	}
	return NewRoom;
}

AStaticMeshActor* ALevelBuilder::GenerateWallAtGrid(FCoord Where, EWallPos Pos)
{
	FTransform RoomLoc = FTransform();
	RoomLoc.SetLocation(GetLocFromGrid(Where));
	AStaticMeshActor* NewWall = GenerateWallAtLoc(RoomLoc, Pos);
	FString ID = GetWallID(Where, Pos);
	AllWalls.Add(ID, NewWall);
	return NewWall;
}

AStaticMeshActor* ALevelBuilder::GenerateEdgeWallAtGrid(FCoord Where, EWallPos Pos)
{
	FCoord SideCoord = GetNeighbor(Where, Pos);
	FGridStruct* Side = Grid.Find(SideCoord);
	if (!Side)
	{
		GenerateWallAtGrid(Where, Pos);
	}
	return nullptr;
}

AStaticMeshActor* ALevelBuilder::GenerateWallAtLoc(FTransform Where, EWallPos Pos)
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

void ALevelBuilder::BuildGrid()
{
	int16 x = 0;
	int16 y = 0;
	for (uint8 i = 0; i < 9; i++)
	{
		// FCoord Coord = {x, y};
		FCoord Coord = FCoord(x, y);
		FGridStruct Content; Content.RoomType = GetRandomRoom();
		Grid.Add(Coord, Content);
		// UE_LOG(LogTemp, Warning, TEXT("Added tile at %d, %d, Grid now has %d"), Coord.X, Coord.Y, Grid.Num());
		
		if (FMath::RandRange(0, 3) == 0) x++;
		else y++;
	}
}

void ALevelBuilder::BuildWalls(TPair<FCoord, FGridStruct> Tile)
{
	// edge of world walls
	for (auto &&Dir : ALLDIRECTIONS)
	{
		GenerateEdgeWallAtGrid(Tile.Key, Dir);
	}
	// room type walls
	if (Tile.Value.RoomType->bIsWalled)
	{
		for (auto &&Dir : ALLDIRECTIONS)
		{
			GenerateWallAtGrid(Tile.Key, Dir);
		}
	}
	
}

FVector ALevelBuilder::GetLocFromGrid(FCoord Coord)
{
	return FVector(2000.0f * (float)Coord.X, 2000.0f * (float)Coord.Y, 0.0f);
}

FString ALevelBuilder::GetWallID(FCoord Coord1, FCoord Coord2)
{
	FString s1 = Coord1.ToString();
	FString s2 = Coord2.ToString();
	if (s1 < s2) return s1 + 'x' + s2;
	else return s2 + 'x' + s1;

	return FString();
}

FString ALevelBuilder::GetWallID(FCoord Coord, EWallPos Dir)
{
	return GetWallID(Coord, GetNeighbor(Coord, Dir));
}

FCoord ALevelBuilder::GetNeighbor(FCoord From, EWallPos To)
{
	return FCoord(From.X + GetXFromDir(To), From.Y + GetYFromDir(To));
}



