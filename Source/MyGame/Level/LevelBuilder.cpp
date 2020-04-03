// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBuilder.h"
#include "Components/BillboardComponent.h"
#include "Engine/LevelStreaming.h"
#include "AssetRegistryModule.h"
#include "RoomDataAsset.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "../MyGameInstance.h"

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
		// UE_LOG(LogTemp, Warning, TEXT("created %s room at %s"), *Tile.Value.RoomType->LevelAddress.ToString(), *Tile.Key.ToString());
		BuildWalls(Tile);
	}
	
	// for (auto &&Wall : AllWalls)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Wall: %s"), *Wall.Key);
	// }
	
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
	ULevelStreaming* NewRoom = OnBPCreateLevelByName(RoomType->GetAutoLevelAddress());
	// UE_LOG(LogTemp, Warning, TEXT("AddressO: %s"), *RoomType->LevelAddress.ToString());
	// UE_LOG(LogTemp, Warning, TEXT("AddressA: %s"), *RoomType->GetAutoLevelAddress().ToString());
	// ULevelStreaming* NewRoom = OnBPCreateLevelByName(RoomType->LevelAddress);
	if (NewRoom)
	{
		NewRoom->LevelTransform.SetLocation(GetLocFromGrid(Where));
		NewRoom->SetShouldBeVisible(true);
		NewRoom->SetShouldBeLoaded(true);
	}
	return NewRoom;
}

AStaticMeshActor* ALevelBuilder::GenerateWallAtGrid(FCoord Where, EWallPos Pos, UStaticMesh* What)
{
	FString ID = GetWallID(Where, Pos);
	AStaticMeshActor** Existing = AllWalls.Find(ID);
	if (Existing) {
		// UE_LOG(LogTemp, Warning, TEXT("Wall of ID: %s, already exists"), *ID);
		return nullptr;
	}
	FTransform RoomLoc = FTransform();
	RoomLoc.SetLocation(GetLocFromGrid(Where));
	AStaticMeshActor* NewWall = GenerateWallAtLoc(RoomLoc, Pos, What);
	AllWalls.Add(ID, NewWall);
	// UE_LOG(LogTemp, Warning, TEXT("Generated wall of ID: %s, total %d walls"), *ID, AllWalls.Num());
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

AStaticMeshActor* ALevelBuilder::GenerateWallAtLoc(FTransform Where, EWallPos Pos, UStaticMesh* What)
{
	FVector Loc = Where.GetLocation();
	FRotator Rot = Where.Rotator();
	switch (Pos)
	{
	case EWallPos::Top:
		Loc.X += 1000.0f;
		break;

	case EWallPos::Bottom:
		Loc.X -= 1000.0f;
		break;
	
	case EWallPos::Left:
		Loc.Y -= 1000.0f;
		Rot.Yaw = 90.0f;
		break;
	
	case EWallPos::Right:
		Loc.Y += 1000.0f;
		Rot.Yaw = 90.0f;
		break;
	}
	return GenerateWall(FTransform(Rot, Loc), What);
}

AStaticMeshActor* ALevelBuilder::GenerateWall(FTransform Where, UStaticMesh* What)
{
	// return OnBPCreateLevelByName("Game/Maps/Rooms/Room01");
	if (What == nullptr) What = WallMesh;
	FVector Loc = Where.GetLocation();
	// Loc.Y += 1000.0f; Loc.Z += 100.0f;
	FActorSpawnParameters params;
	params.bNoFail = true;
	AStaticMeshActor* NewWall = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Loc, Where.Rotator(), params);
	NewWall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Stationary);
	NewWall->GetStaticMeshComponent()->SetStaticMesh(What);
	// NewWall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Stationary);
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

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
	if (!MyGI) return;
	MyGI->SetLevelBuilderRef(this);
}


URoomDataAsset* ALevelBuilder::GetRandomRoom()
{
	if (RoomList.Num() == 0) return nullptr;
	int32 Index = FMath::RandRange(0, RoomList.Num() - 1);
	return RoomList[Index];
}

URoomDataAsset* ALevelBuilder::GetRandomRoom(int32 Difficulty)
{
	if (RoomList.Num() == 0) return nullptr;
	TArray<URoomDataAsset *> FilteredRooms = {};
	for (auto &&Room : RoomList)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Room Difficulty: %d, Need: %d"), Room->RoomDifficulty, Difficulty);
		if (Room->RoomDifficulty == Difficulty)
		{
			FilteredRooms.Add(Room);
		}
	}
	if (FilteredRooms.Num() == 0) return nullptr;
	int32 Index = FMath::RandRange(0, FilteredRooms.Num() - 1);
	return FilteredRooms[Index];
}

void ALevelBuilder::BuildGrid()
{
	int16 x = 0;
	int16 y = 0;
	int32 Difficulty = 0;
	int16 ChanceOfGoingRight = InitialChanceOfGoingRight;
	for (uint8 i = 0; i < NumRooms; i++)
	{
		// FCoord Coord = {x, y};
		FCoord Coord = FCoord(x, y);
		if (i > 1) Difficulty = 1;
		if (i > 3) Difficulty = 2;
		if (i == NumRooms - 2) {ChanceOfGoingRight = 100;}
		if (i == NumRooms - 1) {Difficulty = 9;}
		FGridStruct Content; Content.RoomType = GetRandomRoom(Difficulty);
		Grid.Add(Coord, Content);
		// UE_LOG(LogTemp, Warning, TEXT("Added tile at %d, %d, Grid now has %d for i: %d"), Coord.X, Coord.Y, Grid.Num(), i);
		// y++;
		// UE_LOG(LogTemp, Warning, TEXT("Chance of Going Right: %d%"), ChanceOfGoingRight);
		if (FMath::RandRange(1, 100) <= ChanceOfGoingRight)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Chose Right"));
			y++; ChanceOfGoingRight -= DecWhenChoseRight;
		}
		else 
		{
			// UE_LOG(LogTemp, Warning, TEXT("Chose Vert"));
			ChanceOfGoingRight += IncWhenChoseVert;
			if(FMath::RandBool())
			{
				if(Grid.Find(GetNeighbor(Coord, EWallPos::Top))) x--;
				else x++;
			}
			else 
			{
				if(Grid.Find(GetNeighbor(Coord, EWallPos::Bottom))) x++;
				else x--;
			}
		}

		// else x--;
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
			GenerateWallAtGrid(Tile.Key, Dir, WallDooredMesh);
		}
	}
	
}

FVector ALevelBuilder::GetLocFromGrid(FCoord Coord)
{
	return FVector(2000.0f * (float)Coord.X, 2000.0f * (float)Coord.Y, 0.0f);
}

FCoord ALevelBuilder::GetGridFromLoc(FVector Location)
{
	int16 NewX = FMath::DivideAndRoundNearest(Location.X, 2000.0f);
	int16 NewY = FMath::DivideAndRoundNearest(Location.Y, 2000.0f);
	return FCoord(NewX, NewY);
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

AStaticMeshActor* ALevelBuilder::GetBottomWallFromLoc(FVector Location)
{
	return GetWallRefFromCoordAndDir(GetGridFromLoc(Location), EWallPos::Bottom);
}
	
AStaticMeshActor* ALevelBuilder::GetWallRefFromCoordAndDir(FCoord Coord, EWallPos Dir)
{
	// UE_LOG(LogTemp, Warning, TEXT("Requested wall on %s"), *Coord.ToString());
	FString ID = GetWallID(Coord, Dir);
	AStaticMeshActor** Wall = AllWalls.Find(ID);
	if (Wall) return *Wall;
	return nullptr;
}

void ALevelBuilder::HideWall(FVector Location, EWallPos Dir)
{
	HideWall(GetGridFromLoc(Location), Dir);
}

void ALevelBuilder::HideWall(FCoord Coord, EWallPos Dir)
{
	if (Coord == LastHiddenWallCoord) return;
	LastHiddenWallCoord = Coord;
	// Unhide all Walls
	for (auto &&Wall : HiddenWalls)
	{
		if (Wall && Wall->GetStaticMeshComponent())
		{
			Wall->GetStaticMeshComponent()->SetVisibility(true);
		}
	}
	HiddenWalls = {};
	// remove all capped walls
	for (auto &&Wall : CappedWalls)
	{
		Wall->Destroy();
	}
	CappedWalls = {};
	AStaticMeshActor* SM = GetWallRefFromCoordAndDir(Coord, Dir);
	if (SM && SM->GetStaticMeshComponent())
	{
		// then hide it
		SM->GetStaticMeshComponent()->SetVisibility(false);
		HiddenWalls.Add(SM);
		// UE_LOG(LogTemp, Warning, TEXT("Hidding wall on %s"), *Coord.ToString());
		// and add a capped wall in its place
		FTransform Loc = FTransform();
		Loc.SetLocation(SM->GetActorLocation());
		AStaticMeshActor* CappedWall = GenerateWall(Loc, GetWallTypeAtTiles(Coord, GetNeighbor(Coord, Dir), true));
		// UE_LOG(LogTemp, Warning, TEXT("Wallcap between %s and %s is %s"), *Coord.ToString(), *GetNeighbor(Coord, Dir).ToString(), *CappedWall->GetName());
		CappedWalls.Add(CappedWall);
	}
}

UStaticMesh* ALevelBuilder::GetWallTypeAtTiles(FCoord Coord1, FCoord Coord2, bool Cap)
{
	FGridStruct* Tile1 = Grid.Find(Coord1);
	FGridStruct* Tile2 = Grid.Find(Coord2);
	if (!Tile1 || !Tile2) return Cap? WallCappedMesh : WallMesh;
	if (Tile1->RoomType->bIsWalled || Tile2->RoomType->bIsWalled) return Cap? WallDooredCappedMesh : WallDooredMesh;
	return nullptr;
}






