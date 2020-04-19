// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBuilder.h"
#include "Components/BillboardComponent.h"
#include "Engine/LevelStreaming.h"
#include "AssetRegistryModule.h"
#include "RoomDataAsset.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "../MyGameInstance.h"
#include "Door.h"

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
	// PrimaryActorTick.bCanEverTick = true;

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

// Try to Create a Wall at this Grid Coord to direction Dir. Will not create the wall if it already exists
// if Doored is false, creates just a WallMesh, else if Doored is true, creates a DooredWallMesh and a ADoor Actor
AStaticMeshActor* ALevelBuilder::GenerateWallAtGrid(FCoord Where, EWallPos Dir, bool Doored = false)
{
	AStaticMeshActor* result = nullptr;
	if (Doored)
	{
		result = GenerateWallMeshAtGrid(Where, Dir, WallDooredMesh);
		if (result)
		{
			SpawnDoor(Where, Dir);
		}
	}
	else 
	{
		result = GenerateWallMeshAtGrid(Where, Dir, WallMesh);
	}
	return result;
}
// Try to Create a Wall at this Grid Coord to direction Dir. Will not create the wall if it already exists
AStaticMeshActor* ALevelBuilder::GenerateWallMeshAtGrid(FCoord Where, EWallPos Dir, UStaticMesh* What)
{
	FString ID = GetWallID(Where, Dir);
	AStaticMeshActor** Existing = AllWalls.Find(ID);
	if (Existing) {
		// UE_LOG(LogTemp, Warning, TEXT("Wall of ID: %s, already exists"), *ID);
		return nullptr;
	}
	FTransform RoomLoc = FTransform();
	RoomLoc.SetLocation(GetLocFromGrid(Where));
	AStaticMeshActor* NewWall = GenerateWallAtLoc(RoomLoc, Dir, What);
	AllWalls.Add(ID, NewWall);
	// UE_LOG(LogTemp, Warning, TEXT("Generated wall of ID: %s, total %d walls"), *ID, AllWalls.Num());
	return NewWall;
}

AStaticMeshActor* ALevelBuilder::GenerateEdgeWallAtGrid(FCoord Where, EWallPos Pos)
{
	FCoord SideCoord = GetNeighbor(Where, Pos);
	FRoomState* Side = Grid.Find(SideCoord);
	if (!Side)
	{
		GenerateWallMeshAtGrid(Where, Pos);
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
	FActorSpawnParameters params;
	params.bNoFail = true;
	AStaticMeshActor* NewWall = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Loc, Where.Rotator(), params);
	NewWall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Stationary);
	NewWall->GetStaticMeshComponent()->SetStaticMesh(What);
	// NewWall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Stationary);
	return NewWall;
}

ADoor* ALevelBuilder::SpawnDoor(FCoord Where, EWallPos Dir)
{
	FTransform DoorTran = GetWallLocFromGridAndDir(Where, Dir);
	FVector Loc = DoorTran.GetLocation();
	FActorSpawnParameters params;
	params.bNoFail = true;
	ADoor* NewDoor = GetWorld()->SpawnActor<ADoor>(DoorActor, Loc, DoorTran.Rotator(), params);
	NewDoor->OpenDoor();
	DoorList.Add(NewDoor);
	return NewDoor;
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
		FRoomState Content; Content.RoomType = GetRandomRoom(Difficulty);
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

void ALevelBuilder::BuildWalls(TPair<FCoord, FRoomState> Tile)
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
			// GenerateWallAtGrid(Tile.Key, Dir, WallDooredMesh);
			GenerateWallAtGrid(Tile.Key, Dir, true);
		}
	}
	
}

FVector ALevelBuilder::GetLocFromGrid(FCoord Coord)
{
	return FVector(2000.0f * (float)Coord.X, 2000.0f * (float)Coord.Y, 0.0f);
}

FTransform ALevelBuilder::GetWallLocFromGridAndDir(FCoord Coord, EWallPos Dir)
{
	FVector Loc = GetLocFromGrid(Coord);
	FRotator Rot = FRotator::ZeroRotator;
	switch (Dir)
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
	FTransform result = FTransform(Rot, Loc);
	return result;
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

FRoomState* ALevelBuilder::GetRoomStateFromCoord(FCoord Coord)
{
	// if (!Grid) return nullptr;
	FRoomState* GS = Grid.Find(Coord);
	return GS;
}

URoomDataAsset* ALevelBuilder::GetRoomFromCoord(FCoord Coord)
{
	FRoomState* GS = Grid.Find(Coord);
	return GS->RoomType;
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

// Try to hide walls on the Grid at this Location
// Called from the Character Every tick with the Actor location
void ALevelBuilder::OnUpdateCharCoord(FVector Location, EWallPos Dir)
{
	FCoord Coord = GetGridFromLoc(Location);
	if (Coord == LastEnteredRoomCoord) return;
	LastEnteredRoomCoord = Coord;
	FRoomState* Room = GetRoomStateFromCoord(Coord);
	if (Room && !Room->bIsRoomCleared && Room->RoomType->bIsDoored)
	{
		CloseDoors();
	}
	HideWall(Coord, Dir);
}

// Try to Hide walls on this Grid Tile
void ALevelBuilder::HideWall(FCoord Coord, EWallPos Dir)
{
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
	FRoomState* Tile1 = Grid.Find(Coord1);
	FRoomState* Tile2 = Grid.Find(Coord2);
	if (!Tile1 || !Tile2) return Cap? WallCappedMesh : WallMesh;
	if (Tile1->RoomType->bIsWalled || Tile2->RoomType->bIsWalled) return Cap? WallDooredCappedMesh : WallDooredMesh;
	return nullptr;
}

void ALevelBuilder::SetRoomClearedAtLoc(FVector Location)
{
	FRoomState* Room = GetRoomStateFromCoord(GetGridFromLoc(Location));
	if (Room)
	{
		Room->bIsRoomCleared = true;
	}
	OpenDoors();
}

void ALevelBuilder::OpenDoors()
{
	for (auto &&Door : DoorList)
	{
		Door->OpenDoor();
	}
}

void ALevelBuilder::CloseDoors()
{
	for (auto &&Door : DoorList)
	{
		Door->CloseDoor();
	}
}



