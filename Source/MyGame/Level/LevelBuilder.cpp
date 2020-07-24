// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBuilder.h"
#include "Door.h"
#include "Wall.h"
#include "RoomMaster.h"
#include "../MyGameInstance.h"
#include "../MyGameInstance.h"

#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/LevelStreaming.h"
#include "AssetRegistryModule.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Math/RandomStream.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

TArray<EDirection> ALLDIRECTIONS = {EDirection::Left, EDirection::Right, EDirection::Bottom, EDirection::Top};
int8 GetXFromDir(EDirection Dir)
{
	if (Dir == EDirection::Top) return 1;
	if (Dir == EDirection::Bottom) return -1;
	return 0;
}
int8 GetYFromDir(EDirection Dir)
{
	if (Dir == EDirection::Right) return 1;
	if (Dir == EDirection::Left) return -1;
	return 0;
}


// Sets default values
ALevelBuilder::ALevelBuilder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// PrimaryActorTick.bCanEverTick = true;
	BBComp = CreateDefaultSubobject<UBillboardComponent>(TEXT("Dino"));
	RootComponent = BBComp;
	RoomBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	// RoomBounds->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	
	if (!WallActor) WallActor = AWall::StaticClass();
	// BBComp->SetupAttachment(RootComponent);

}

void ALevelBuilder::OnConstruction(const FTransform & Transform)
{
	RoomBounds->SetBoxExtent(FVector(RoomSizeX/2, RoomSizeY/2, 100.f));
	RoomBounds->SetWorldLocation(FVector(0.f, 0.f, -100.f));
}

// Called when the game starts or when spawned
void ALevelBuilder::BeginPlay()
{
	Super::BeginPlay();
	// ULevelStreaming* NewLevel = OnBPCreateLevelByName(NewRoom->LevelAddress);

	GetWorldSettings()->SetTimeDilation(0.0f);
	// RandomStream = FRandomStream(RandomSeed);
	// if (!RandomSeed) RandomStream.GenerateNewSeed();

	SetAssetListFromRegistry();
	BuildGrid();
	SpawnLevels();

	if (LevelMusic) LevelMusicRef = UGameplayStatics::SpawnSound2D(GetWorld(), LevelMusic);
	UE_LOG(LogTemp, Warning, TEXT("Grid: %s"), *DebugGrid());
	// GenerateRoom();
}

/* Spawns all streaming levels from the pre-built Grid into the world */
void ALevelBuilder::BuildGrid()
{
	// UE_LOG(LogTemp, Warning, TEXT("Random Seed: %d Stream: %s"), RandomStream.GetCurrentSeed(), *RandomStream.ToString());
	// UE_LOG(LogTemp, Warning, TEXT("Random numbers: %d, %d, %d"), RandomStream.RandRange(0, 10), RandomStream.RandRange(0, 10), RandomStream.RandRange(0, 10));
	int16 x = 0;
	int16 y = 0;
	int32 Difficulty = 0;
	int16 ChanceOfGoingRight = InitialChanceOfGoingRight;
	UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
	if (!GI) return;
	uint8 GIDifficulty = GI->LevelDifficulty;
	NumRooms += GIDifficulty;
	RandomStream = &GI->RandomStream;
	uint8 DooredRoom = 0; 
	
	// uint8 TreasureRoomPosition = 3;
	// uint8 TreasureRoomPosition = FMath::RandRange(1, NumRooms - 2);
	// FCoord TreasureRoomCoord;

	for (uint8 i = 0; i < NumRooms; i++)
	{
		// FCoord Coord = {x, y};
		FCoord Coord = FCoord(x, y);
		if (i > 1) Difficulty = 1;
		if (i > 4) Difficulty = 2;
		if (i > 6) Difficulty = 3;
		if (i == NumRooms - 2) { ChanceOfGoingRight = 100; }
		if (i == NumRooms - 1) { Difficulty = 9; }
		// if (i == TreasureRoomPosition) TreasureRoomCoord = Coord;
		if (NumRooms > 3 && i % 5 == 0)
		{
			DooredRoom = RandomStream->RandRange(0, 4) + FMath::FloorToInt(i / 5) * 5;
			DooredRoom = FMath::Clamp((int)DooredRoom, 2, NumRooms -2);
			UE_LOG(LogTemp, Warning, TEXT("DooredRoom = %d, i = %d"), DooredRoom, i);
		}
		FRoomState Content; 
		// first room will always be empty
		if (i == 0) Content.RoomType = GetRandomRoom(Difficulty, ERoomType::Empty);
		else
		{
			// all next rooms will always have regular enemies, unless it's the last room, then it's a boss room
			if (i == NumRooms - 1) { Content.RoomType = GetRandomRoom(Difficulty, ERoomType::Boss); }
			//else Content.RoomType = GetRandomRoom(Difficulty, ERoomType::Enemies, -1);
			else if (DooredRoom > 0 && i == DooredRoom) Content.RoomType = GetRandomRoom(Difficulty, ERoomType::Enemies, 1);
			//else if (i > 1) Content.RoomType = GetRandomRoom(Difficulty, ERoomType::Enemies, 1);
			else Content.RoomType = GetRandomRoom(Difficulty, ERoomType::Enemies, 0);
		}
		Grid.Add(Coord, Content);
		// UE_LOG(LogTemp, Warning, TEXT("Added tile at %d, %d, Grid now has %d for i: %d"), Coord.X, Coord.Y, Grid.Num(), i);
		// y++;
		// UE_LOG(LogTemp, Warning, TEXT("Chance of Going Right: %d%"), ChanceOfGoingRight);
		// if (FMath::RandRange(1, 100) <= ChanceOfGoingRight)
		if (RandomStream->RandRange(1, 100) <= ChanceOfGoingRight)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Chose Right"));
			y++; ChanceOfGoingRight -= DecWhenChoseRight;
		}
		else
		{
			// UE_LOG(LogTemp, Warning, TEXT("Chose Vert"));
			ChanceOfGoingRight += IncWhenChoseVert;
			// if(FMath::RandBool())
			if (RandomStream->RandRange(0, 1))
			{
				if (IsNeighborFree(Coord, EDirection::Top)) x++;
				else x--;
			}
			else
			{
				if (IsNeighborFree(Coord, EDirection::Bottom)) x--;
				else x++;
			}
		}

		// else x--;
	}
	AddTreasureRoom();
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

/// <summary>
/// Spawns all rooms from the Grid then spawns walls. Calls SpawnRoom for each Room on the Grid.
/// Called from BeginPlay after BuildGrid()
/// </summary>
void ALevelBuilder::SpawnLevels()
{
	FTransform RoomLoc = FTransform();
	for (auto &&Tile : Grid)
	{
		ULevelStreaming* NewRoom = SpawnRoom(Tile.Key, Tile.Value.RoomType);
		Tile.Value.RoomRef = NewRoom;
		// UE_LOG(LogTemp, Warning, TEXT("created %s room at %s"), *Tile.Value.RoomType->LevelAddress.ToString(), *Tile.Key.ToString());
		BuildWalls(Tile);
		FString RoomAddress = "invalid";
		if (Tile.Value.RoomType) RoomAddress = Tile.Value.RoomType->LevelAddress.ToString();
		FString line = FString::Printf(TEXT("Coord: %s, Room: %s, Walls: %d"), *Tile.Key.ToString(), *RoomAddress, Tile.Value.Walls.Num());
		UE_LOG(LogTemp, Warning, TEXT("%s"), *line);
	}
	
}

/* Spawns one room at Where Coords using RoomType DataAsset. Will load the streaming level for
that DataAsset, load it and set it visible to spawn it in the world. Returns a pointer to the
Streaming Level that it spawned. */
ULevelStreaming* ALevelBuilder::SpawnRoom(FCoord Where, class URoomDataAsset* RoomType)
{
	ULevelStreaming* NewRoom = nullptr;
	if (RoomType) NewRoom = OnBPCreateLevelByName(RoomType->GetAutoLevelAddress());
	// UE_LOG(LogTemp, Warning, TEXT("AddressO: %s"), *RoomType->LevelAddress.ToString());
	// UE_LOG(LogTemp, Warning, TEXT("AddressA: %s"), *RoomType->GetAutoLevelAddress().ToString());
	// ULevelStreaming* NewRoom = OnBPCreateLevelByName(RoomType->LevelAddress);
	if (NewRoom)
	{
		NewRoom->LevelTransform.SetLocation(GetLocFromGrid(Where));
		// NewRoom->LevelTransform.SetScale3D(FVector(0.1f, 2.f, 1.0f));
		// NewRoom->LevelTransform.SetToRelativeTransform();
		// NewRoom->LevelTransform.SetRotation(FQuat(FRotator(45.f, 45.f, 45.f)));
		// NewRoom->LevelTransform.SetScale3D(FVector(RoomSizeX/RoomType->RoomSize.X, RoomSizeY/RoomType->RoomSize.Y, 1.0f));
		NewRoom->SetShouldBeVisible(true);
		NewRoom->SetShouldBeLoaded(true);
		// UE_LOG(LogTemp, Warning, TEXT("Loaded new level, is loaded: %d"), NewRoom->IsLevelLoaded());
		NewRoom->OnLevelLoaded.AddDynamic(this, &ALevelBuilder::OnLoadedOneLevel);
		CountOfLevelsThatDidntFinishLoading++;
	}
	return NewRoom;
}

/* Callback function bound to 'OnLevelLoaded' event of a newly loaded room, this function will be called 
when the level actually finishes loading. Because the event has no parameters, it's impossible to know which
level exactly finished loading. So we have to check all of them, every time. */
void ALevelBuilder::OnLoadedOneLevel()
{
	CountOfLevelsThatDidntFinishLoading--;
	if (CountOfLevelsThatDidntFinishLoading == 0)
	{
		GetWorldSettings()->SetTimeDilation(1.0f);
	}
	// UE_LOG(LogTemp, Warning, TEXT("new level is loaded, %d levels left to load"), CountOfLevelsThatDidntFinishLoading);
}

/// <summary>
/// Spawns the walls for one Tile of the Grid.
/// Called by SpawnLevels()
/// </summary>
/// <param name="Tile">One Tile of the Grid Map, contains the Coord the Room is at and the State of the Room</param>
void ALevelBuilder::BuildWalls(TPair<FCoord, FRoomState> &Tile)
{
	// edge of world walls
	for (auto&& Dir : ALLDIRECTIONS)
	{
		auto NewWall = TrySpawnEdgeWallAtCoord(Tile, Dir);
	}
	// room type walls
	if (Tile.Value.RoomType && Tile.Value.RoomType->bIsWalled)
	{
		for (auto&& Dir : ALLDIRECTIONS)
		{
			// GenerateWallAtGrid(Tile.Key, Dir, WallDooredMesh);
			auto NewWall = TrySpawnWallCoordDir(Tile, Dir, true);
		}
	}
}

// Try to Spawn a Wall at this Tile on the Grid in direction Dir. Will not create the wall if it already exists
// Creates a WallSettings and set its Doored type and calls TryToGenerateWallAtGrid with that Settings
AWall* ALevelBuilder::TrySpawnWallCoordDir(TPair<FCoord, FRoomState> &Tile, EDirection Dir, bool Doored = false)
{
	FCoord Where = Tile.Key;
	AWall* result = nullptr;
	FWallSettings NewSettings = FWallSettings();
	FWallSettings* Settings = &NewSettings;
	Settings->bIsDoored = Doored;
	if (Doored)
	{
		result = TrySpawnWallFromSettings(Tile, Dir, Settings);
		if (result)
		{
			SpawnDoor(Where, Dir);
		}
	}
	else 
	{
		result = TrySpawnWallFromSettings(Tile, Dir, Settings);
	}
	return result;
}
// Try to Create a Wall at this Grid Coord to direction Dir. Will not create the wall if it already exists
AWall* ALevelBuilder::TrySpawnWallFromSettings(TPair<FCoord, FRoomState>& Tile, EDirection Dir, FWallSettings* Settings)
{
	FCoord Where = Tile.Key;
	FString ID = GetWallID(Where, Dir);
	AWall** Existing = AllWalls.Find(ID);
	if (Existing) {
		// UE_LOG(LogTemp, Warning, TEXT("Wall of ID: %s, already exists"), *ID);
		return nullptr;
	}
	FTransform RoomLoc = FTransform();
	RoomLoc.SetLocation(GetLocFromGrid(Where));
	AWall* NewWall = SpawnWallAtLocDirSettings(RoomLoc, Dir, Settings);
	if (NewWall)
	{
		AllWalls.Add(ID, NewWall);
		//UE_LOG(LogTemp, Warning, TEXT("Adding wall at %s, Dir: %d"), *Where.ToString(), Dir);
		if (!Tile.Value.Walls.Contains(Dir)) Tile.Value.Walls.Add(Dir);
		FRoomState* Neighbor = Grid.Find(GetNeighbor(Where, Dir));
		EDirection OppositeDir = GetOppositeDirection(Dir);
		if (Neighbor && !Neighbor->Walls.Contains(OppositeDir)) Neighbor->Walls.Add(OppositeDir);
	}
	// UE_LOG(LogTemp, Warning, TEXT("Generated wall of ID: %s, total %d walls"), *ID, AllWalls.Num());
	return NewWall;
}

/// <summary>
/// Try to spawn an Edge AWall at Where Coord and Pos direction. Fails if it already exists.
/// </summary>
/// <param name="Tile">Tile Coordinate to spawn the Wall at</param>
/// <param name="Pos">Direction from Coordinate to Spawn Wall at</param>
/// <returns>The spawned AWall if successful, nullptr if it already exists</returns>
AWall* ALevelBuilder::TrySpawnEdgeWallAtCoord(TPair<FCoord, FRoomState> &Tile, EDirection Pos)
{
	FCoord Where = Tile.Key;
	FCoord SideCoord = GetNeighbor(Where, Pos);
	FRoomState* Side = Grid.Find(SideCoord);
	if (!Side)
	{
		AWall* NewWall = TrySpawnWallCoordDir(Tile, Pos);
		if (NewWall) return NewWall;
	}
	return nullptr;
}

/* Spawns a wall in the room centered in Transform Where, the wall will be at Direction Pos of that room
Will use given Wall Settings but will edit the Settings length depending on Room Size */
AWall* ALevelBuilder::SpawnWallAtLocDirSettings(FTransform Where, EDirection Pos, FWallSettings* Settings)
{
	FVector Loc = Where.GetLocation();
	FRotator Rot = Where.Rotator();
	if (!Settings) 
	{
		FWallSettings NewSettings = FWallSettings();
		Settings = &NewSettings;
	}
	switch (Pos)
	{
	case EDirection::Top:
		Loc.X += RoomSizeX/2;
		Settings->Length = RoomSizeY;
		break;

	case EDirection::Bottom:
		Loc.X -= RoomSizeX/2;
		Settings->Length = RoomSizeY;
		break;
	
	case EDirection::Left:
		Loc.Y -= RoomSizeY/2;
		Rot.Yaw = 90.0f;
		Settings->Length = RoomSizeX;
		break;
	
	case EDirection::Right:
		Loc.Y += RoomSizeY/2;
		Rot.Yaw = 90.0f;
		Settings->Length = RoomSizeX;
		break;
	}
	return SpawnWall(FTransform(Rot, Loc), Settings);
}

/* Spawns a new AWall exactly at World Transform Where with given Wall Settings */
AWall* ALevelBuilder::SpawnWall(FTransform Where, FWallSettings* Settings)
{
	// return OnBPCreateLevelByName("Game/Maps/Rooms/Room01");
	if (Settings == nullptr) 
	{
		FWallSettings NewSettings = FWallSettings();
		Settings = &NewSettings;
	}
	FVector Loc = Where.GetLocation();
	FActorSpawnParameters params;
	params.bNoFail = true;
	AWall* NewWall = GetWorld()->SpawnActor<AWall>(WallActor, Loc, Where.Rotator(), params);
	NewWall->Length = Settings->Length;
	NewWall->bIsDoored = Settings->bIsDoored;
	NewWall->BuildWalls();
	// NewWall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Stationary);
	// NewWall->GetStaticMeshComponent()->SetStaticMesh(What);
	// NewWall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Stationary);
	return NewWall;
}

ADoor* ALevelBuilder::SpawnDoor(FCoord Where, EDirection Dir)
{
	FTransform DoorTran = GetWallLocFromGridAndDir(Where, Dir);
	FVector Loc = DoorTran.GetLocation();
	FActorSpawnParameters params;
	params.bNoFail = true;
	ADoor* NewDoor = GetWorld()->SpawnActor<ADoor>(DoorActor, Loc, DoorTran.Rotator(), params);
	NewDoor->OpenDoor();
	DoorList.Add(NewDoor);
	if (IsTileOfType(Where, ERoomType::Treasure) || IsTileOfType(GetNeighbor(Where, Dir), ERoomType::Treasure))
	{
		NewDoor->SetSymbol(ERoomType::Treasure);
	}
	if (IsTileOfType(Where, ERoomType::Boss) || IsTileOfType(GetNeighbor(Where, Dir), ERoomType::Boss))
	{
		NewDoor->SetSymbol(ERoomType::Boss);
	}
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

/* Adds a Room of ERoomType::Treasure to a free Grid Coord that is a neighbor of a random Grid Tile 
between the Index 1 and second last of the Grid. Will try 20 times. If it fails, returns nullptr.
Will add the Room to the Grid */
URoomDataAsset* ALevelBuilder::AddTreasureRoom()
{
	URoomDataAsset* result = nullptr;
	if (NumRooms < 3) return result;
	TArray<FCoord> GridCoords; Grid.GetKeys(GridCoords);
	for (uint8 i = 0; i < 20; i++)
	{
		// uint8 TreasurePosition = FMath::RandRange(2, GridCoords.Num() - 2);
		uint8 TreasurePosition = RandomStream->RandRange(2, GridCoords.Num() - 2);
		FCoord Coord = GridCoords[TreasurePosition];
		result = AddTreasureRoomNextTo(Coord);
		if (result) return result;
	}
	return result;
}

/* Adds a Room of ERoomType::Treasure to a free Grid Coord that is a free neighbor from given Coord param.
Will add the Room to the Grid */
URoomDataAsset* ALevelBuilder::AddTreasureRoomNextTo(FCoord Coord)
{
	TArray<URoomDataAsset *> FilteredRooms = FindRoomsOfType(ERoomType::Treasure);
	if (FilteredRooms.Num() == 0) 
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Treasure room, no free space next to %s"), *Coord.ToString());
		return nullptr;
	}
	FRoomState Content; Content.RoomType = FilteredRooms[0];
	TArray<FCoord> FreeCoords = FindFreeNeighbors(Coord);
	FCoord TreasureCoord = FreeCoords[0];
	if (IsAnyNeighborOfType(TreasureCoord, ERoomType::Boss)) 
	{
		return nullptr;
	}
	Grid.Add(TreasureCoord, Content);
	UE_LOG(LogTemp, Warning, TEXT("Added treasure at %s"), *TreasureCoord.ToString());
	return Content.RoomType;
}

/// <summary>
/// Returns a pointer to a room of random type. Of any room type and any difficulty
/// </summary>
/// <returns></returns>
URoomDataAsset* ALevelBuilder::GetRandomRoom()
{
	if (RoomList.Num() == 0) return nullptr;
	int32 Index = RandomStream->RandRange(0, RoomList.Num() - 1);
	// int32 Index = FMath::RandRange(0, RoomList.Num() - 1);
	return RoomList[Index];
}

/// <summary>
/// Returns a pointer to a room of random type. Of any type but always of specified difficulty
/// </summary>
/// <param name="Difficulty"></param>
/// <returns></returns>
URoomDataAsset* ALevelBuilder::GetRandomRoom(int32 Difficulty)
{
	if (RoomList.Num() == 0) return nullptr;
	TArray<URoomDataAsset*> FilteredRooms = {};
	FilteredRooms = FindRoomsOfDifficulty(Difficulty);
	if (FilteredRooms.Num() == 0) return nullptr;
	int32 Index = RandomStream->RandRange(0, FilteredRooms.Num() - 1);
	// int32 Index = FMath::RandRange(0, FilteredRooms.Num() - 1);

	return FilteredRooms[Index];
}

/// <summary>
/// Returns a pointer to a random room of specified type and specified difficulty. If difficulty is less than zero, then ignore difficulty
/// </summary>
/// <param name="Difficulty"></param>
/// <param name="Type"></param>
/// <returns></returns>
URoomDataAsset* ALevelBuilder::GetRandomRoom(int32 Difficulty, ERoomType Type, int8 IsDoored)
{
	if (RoomList.Num() == 0) return nullptr;
	TArray<URoomDataAsset*> FilteredRooms = {};
	FilteredRooms = FindRoomsOfType(Type, Difficulty, IsDoored);
	if (FilteredRooms.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find rooms, Difficulty: %d, Type: %d, Doored: %d"), Difficulty, Type, IsDoored);
		return nullptr;
	}
	int32 Index = RandomStream->RandRange(0, FilteredRooms.Num() - 1);
	// int32 Index = FMath::RandRange(0, FilteredRooms.Num() - 1);

	return FilteredRooms[Index];
}


/**Returns a filtered list of Rooms from RoomList that contains only Rooms of this Difficulty.
*/
TArray<URoomDataAsset*> ALevelBuilder::FindRoomsOfDifficulty(int32 Difficulty)
{
	TArray<URoomDataAsset *> FilteredRooms = {};
	if (RoomList.Num() == 0) return FilteredRooms;
	for (auto &&Room : RoomList)
	{
		if (Room->RoomDifficulty == Difficulty && Room->RoomType != ERoomType::Treasure)
		{
			FilteredRooms.Add(Room);
		}
	}
	return FilteredRooms;
}

/**Returns a filtered list of Rooms from RoomList that contains only Rooms of this Type
 * and Difficulty. If Difficulty is less than zero then ignores Difficulty.
 * if IsDoored is zero or 1, then only find rooms that are not doored, or only that are doored. -1 to ignore
*/
TArray<URoomDataAsset*> ALevelBuilder::FindRoomsOfType(ERoomType Type, int32 Difficulty, int8 IsDoored)
{
	TArray<URoomDataAsset *> FilteredRooms = {};
	if (RoomList.Num() == 0) return FilteredRooms;
	for (auto &&Room : RoomList)
	{
		if (Type == Room->RoomType && (Difficulty < 0 || Room->RoomDifficulty == Difficulty) && (IsDoored < 0 || Room->bIsDoored == (bool)IsDoored))
		{
			FilteredRooms.Add(Room);
		}
	}
	return FilteredRooms;
}
FString ALevelBuilder::DebugGrid()
{
	FString result = FString();
	for (auto&& Tile : Grid)
	{
		FString RoomAddress = "invalid";
		if (Tile.Value.RoomType) RoomAddress = Tile.Value.RoomType->LevelAddress.ToString();
		FString line = FString::Printf(TEXT("Coord: %s, Room: %s, Walls: %d"), *Tile.Key.ToString(), *RoomAddress, Tile.Value.Walls.Num());
		UE_LOG(LogTemp, Warning, TEXT("%s"), *line);
		result.Append(line);
	}
	return result;
}
FVector ALevelBuilder::GetLocFromGrid(FCoord Coord)
{
	return FVector(RoomSizeX * (float)Coord.X, RoomSizeY * (float)Coord.Y, 0.0f);
}

FTransform ALevelBuilder::GetWallLocFromGridAndDir(FCoord Coord, EDirection Dir)
{
	FVector Loc = GetLocFromGrid(Coord);
	FRotator Rot = FRotator::ZeroRotator;
	switch (Dir)
	{
	case EDirection::Top:
		Loc.X += RoomSizeX/2;
		break;

	case EDirection::Bottom:
		Loc.X -= RoomSizeX/2;
		break;
	
	case EDirection::Left:
		Loc.Y -= RoomSizeY/2;
		Rot.Yaw = 90.0f;
		break;
	
	case EDirection::Right:
		Loc.Y += RoomSizeY/2;
		Rot.Yaw = 90.0f;
		break;
	}
	FTransform result = FTransform(Rot, Loc);
	return result;
}

FCoord ALevelBuilder::GetGridFromLoc(FVector Location)
{
	int16 NewX = FMath::DivideAndRoundNearest(Location.X, RoomSizeX);
	int16 NewY = FMath::DivideAndRoundNearest(Location.Y, RoomSizeY);
	return FCoord(NewX, NewY);
}

/// <summary>
/// From a given CurrentLocation returns the center of the Room that CurrentLocation is in
/// </summary>
/// <param name="CurrentLocation"></param>
/// <returns></returns>
FVector ALevelBuilder::GetLocOfRoomCenter(FVector CurrentLocation)
{
	FCoord Coord = GetGridFromLoc(CurrentLocation);
	return GetLocFromGrid(Coord);
}

FString ALevelBuilder::GetWallID(FCoord Coord1, FCoord Coord2)
{
	FString s1 = Coord1.ToString();
	FString s2 = Coord2.ToString();
	if (s1 < s2) return s1 + 'x' + s2;
	else return s2 + 'x' + s1;

	return FString();
}

FString ALevelBuilder::GetWallID(FCoord Coord, EDirection Dir)
{
	return GetWallID(Coord, GetNeighbor(Coord, Dir));
}

/* Returns the Grid Coord of the Neighbor of this Coord From to direction To
 */
FCoord ALevelBuilder::GetNeighbor(FCoord From, EDirection To)
{
	return FCoord(From.X + GetXFromDir(To), From.Y + GetYFromDir(To));
}

/* Returns true if the neighbor From this Coord To this direction does not have any 
Rooms in the Grid yet
 */
bool ALevelBuilder::IsNeighborFree(FCoord From, EDirection To)
{
	FCoord NeighborCoord = GetNeighbor(From, To);
	// if(Grid.Contains(NeighborCoord)) true;
	return !Grid.Contains(NeighborCoord);
}

/* Returns an Array with all the Grid Coordinates of free neighbors From this Grid Coord.
Free Neighbors are Grid Coords that don't have any Room yet */
TArray<FCoord> ALevelBuilder::FindFreeNeighbors(FCoord From)
{
	TArray<FCoord> result = {};
	for (auto &&Dir : ALLDIRECTIONS)
	{
		if (IsNeighborFree(From, Dir)) result.Add(GetNeighbor(From, Dir));
	}
	return result;
}

/* Returns true if any of the Neighbors From given Coord is of given Room Type */
bool ALevelBuilder::IsAnyNeighborOfType(FCoord From, ERoomType Type)
{
	TArray<FCoord> NeighborCoords = GetAllNeighborsCoords(From);
	for (auto &&Neighbor : NeighborCoords)
	{
		FRoomState* NeighborState = Grid.Find(Neighbor);
		if (!NeighborState || !NeighborState->RoomType) continue;
		if (NeighborState->RoomType->RoomType == Type) return true;
	}
	return false;
}
/* Returns true if this Tile at this Coord is of given Room Type */
bool ALevelBuilder::IsTileOfType(FCoord Tile, ERoomType Type)
{
	FRoomState* MyState = Grid.Find(Tile);
	if (!MyState || !MyState->RoomType) return false;
	if (MyState->RoomType->RoomType == Type) return true;
	return false;
}

/* Returns the Coords of all Neighboring Tiles in the Grid From this given Grid Coord */
TArray<FCoord> ALevelBuilder::GetAllNeighborsCoords(FCoord From)
{
	TArray<FCoord> result = {};
	for (auto &&Dir : ALLDIRECTIONS)
	{
		result.Add(GetNeighbor(From, Dir));
	}
	return result;
}

/// <summary>
/// Returns what the EDirection is the opposite of From. For example, Left is opposite of Right.
/// Used by wall spawning to figure what direction to add wall data to both tiles the wall is between.
/// </summary>
/// <param name="From">Direction to get the opposite from</param>
/// <returns>The opposite direction of From</returns>
EDirection ALevelBuilder::GetOppositeDirection(EDirection From)
{
	switch (From)
	{
	case EDirection::Left:
		return EDirection::Right;
	case EDirection::Right:
		return EDirection::Left;
	case EDirection::Bottom:
		return EDirection::Top;
	case EDirection::Top:
		return EDirection::Bottom;
	default:
		break;
	}
	return EDirection();
}

FRoomState* ALevelBuilder::GetRoomStateFromCoord(FCoord Coord)
{
	// if (!Grid) return nullptr;
	FRoomState* GS = Grid.Find(Coord);
	return GS;
}

FRoomState* ALevelBuilder::GetRoomStateFromLoc(FVector Location)
{
	return GetRoomStateFromCoord(GetGridFromLoc(Location));
}

/// <summary>
/// Registers a RoomMaster in the RoomState of the Room in this Location.
/// Called by the RoomMater at its BeginPlay
/// </summary>
/// <param name="NewRoomMaster">The RoomMaster to register</param>
/// <param name="Location">Where the RoomMaster is so I can place its reference in the appropriate Room in the Grid.</param>
void ALevelBuilder::RegisterRoomMaster(ARoomMaster* NewRoomMaster, FVector Location)
{
	FRoomState* Room = GetRoomStateFromCoord(GetGridFromLoc(Location));
	Room->RoomMasterRef = NewRoomMaster;
	NewRoomMaster->RoomStateRef = Room;
}

/// <summary>
/// Called when player enters a new room. Broadcasts RoomEnterDelegate.
/// Called by OnUpdateCharCoord if curent room is different from last.
/// </summary>
/// <param name="NewRoom">State of the Room player is currently at</param>
void ALevelBuilder::OnEnterRoom(FRoomState NewRoom)
{
	if (NewRoom.RoomType && NewRoom.RoomType->RoomType == ERoomType::Boss && BossMusic)
	{
		if (LevelMusic && LevelMusicRef) LevelMusicRef->Stop();
		if (BossMusic && !BossMusicRef) BossMusicRef = UGameplayStatics::SpawnSound2D(GetWorld(), BossMusic);
	}

	OnEnterRoomDelegate.Broadcast(NewRoom);
}

URoomDataAsset* ALevelBuilder::GetRoomFromCoord(FCoord Coord)
{
	FRoomState* GS = Grid.Find(Coord);
	if (GS) return GS->RoomType;
	return nullptr;
}

AWall* ALevelBuilder::GetBottomWallFromLoc(FVector Location)
{
	return GetWallRefFromCoordAndDir(GetGridFromLoc(Location), EDirection::Bottom);
}
	
AWall* ALevelBuilder::GetWallRefFromCoordAndDir(FCoord Coord, EDirection Dir)
{
	// UE_LOG(LogTemp, Warning, TEXT("Requested wall on %s"), *Coord.ToString());
	FString ID = GetWallID(Coord, Dir);
	AWall** Wall = AllWalls.Find(ID);
	if (Wall) return *Wall;
	return nullptr;
}

// Try to hide walls on the Grid at this Location
// Called from the Character Every tick with the Actor location
void ALevelBuilder::OnUpdateCharCoord(FVector Location, EDirection Dir)
{
	FCoord Coord = GetGridFromLoc(Location);
	if (Coord == LastEnteredRoomCoord) return;
	LastEnteredRoomCoord = Coord;
	UE_LOG(LogTemp, Warning, TEXT("Char at location %s, which is coord %s"), *Location.ToString(), *Coord.ToString());
	FRoomState* Room = GetRoomStateFromCoord(Coord);
	// this might actually be null if I dash into a pit UNDER the wall like an idiot
	if (!Room) return;
	// TODO pass location as parameter
	OnEnterRoom(*Room);
	ARoomMaster* Master = Room->RoomMasterRef;
	if (!Master) return;
	//UE_LOG(LogTemp, Warning, TEXT("Room? %d, Room: %s, isDoored: %d"), (Room != nullptr), *Room->RoomType->LevelAddress.ToString(), Room->RoomType->bIsDoored);
	if (!Room->bIsRoomCleared && Room->RoomType->bIsDoored && !Master->AreAllCharsDead())
	{
		//UE_LOG(LogTemp, Warning, TEXT("closing doors"));
		TeleportPlayerInsideRoom(Location);
		//DebugGrid();
		if (Master) Master->bIsDoorOpen = false;
		CloseDoors();
	}
	HideWall(Coord, Dir);
}

// Try to Hide walls on this Grid Tile
void ALevelBuilder::HideWall(FCoord Coord, EDirection Dir)
{
	// Unhide all Walls
	for (auto &&Wall : HiddenWalls)
	{
		if (Wall)
		{
			Wall->Height = 400.f;
			Wall->BuildWalls();
		}
	}
	HiddenWalls = {};
	AWall* ThisWall = GetWallRefFromCoordAndDir(Coord, Dir);
	// Hide wall in given coord
	HideOneWall(ThisWall);
	FCoord RightCoord = GetNeighbor(Coord, EDirection::Right);
	AWall* RightWall = GetWallRefFromCoordAndDir(RightCoord, Dir);
	URoomDataAsset* RightRoom = GetRoomFromCoord(RightCoord);
	// if room to the right is not doored, then hide it too
	if (RightRoom && !RightRoom->bIsDoored)	HideOneWall(RightWall);
	FCoord LeftCoord = GetNeighbor(Coord, EDirection::Left);
	AWall* LeftWall = GetWallRefFromCoordAndDir(LeftCoord, Dir);
	URoomDataAsset* LeftRoom = GetRoomFromCoord(LeftCoord);
	// if room to the left is not doored, then hide it too
	if (LeftRoom && !LeftRoom->bIsDoored) HideOneWall(LeftWall);

}

void ALevelBuilder::HideOneWall(AWall* ThisWall)
{
	if (ThisWall)
	{
		ThisWall->Height = 50.f;
		ThisWall->BuildWalls();
		HiddenWalls.Add(ThisWall);
	}
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

/// <summary>
/// Closes all Doors from the DoorList Array
/// </summary>
void ALevelBuilder::CloseDoors()
{
	for (auto &&Door : DoorList)
	{
		Door->CloseDoor();
	}
}

/// <summary>
/// Teleports the player inside the current room. Called before closing the doors of that room to ensure the
/// Character isn't locked outside
/// </summary>
/// <param name="OldLocation">Where the Char was before being teleported</param>
void ALevelBuilder::TeleportPlayerInsideRoom(FVector OldLocation)
{
	FCoord Coord = GetGridFromLoc(OldLocation);
	FVector NewLocation = GetLocFromGrid(Coord);
	NewLocation.Z = OldLocation.Z;
	float Dist = 60.f;

	// try to find which door I came from
	float DistTop = NewLocation.X - OldLocation.X;
	float DistRight = NewLocation.Y - OldLocation.Y;
	if (FMath::Abs(DistRight) > FMath::Abs(DistTop))
	{
		// this means I came from either right or left
		if (NewLocation.Y > OldLocation.Y) NewLocation.Y -= (RoomSizeY / 2 - Dist);	// from left
		else NewLocation.Y += (RoomSizeY / 2 - Dist);								// from right
		NewLocation.X = OldLocation.X;
	}
	else
	{
		// this means I came from either top or bottom
		if (NewLocation.X > OldLocation.X) NewLocation.X -= (RoomSizeX / 2 - Dist);	// from bottom
		else NewLocation.X += (RoomSizeX / 2 - Dist);								// from top
		NewLocation.Y = OldLocation.Y;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
	if (!MyGI) return;
	MyGI->TeleportPlayer(NewLocation);


}

/// <summary>
/// Aggro all enemies in the room at this Location to the given player
/// </summary>
/// <param name="Location">Location of the Room</param>
void ALevelBuilder::AggroRoom(APawn* PlayerToAggro, FVector Location)
{
	FRoomState* Room = GetRoomStateFromCoord(GetGridFromLoc(Location));
	if (!Room || !Room->RoomMasterRef) return;
	Room->RoomMasterRef->AggroAll(PlayerToAggro);
}



