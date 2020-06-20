// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBuilder.h"
#include "Door.h"
#include "Wall.h"
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
	if (GI)
	{
		uint8 GIDifficulty = GI->LevelDifficulty;
		NumRooms += GIDifficulty;
		RandomStream = &GI->RandomStream;
	}
	else return;
	// uint8 TreasureRoomPosition = 3;
	// uint8 TreasureRoomPosition = FMath::RandRange(1, NumRooms - 2);
	// FCoord TreasureRoomCoord;

	for (uint8 i = 0; i < NumRooms; i++)
	{
		// FCoord Coord = {x, y};
		FCoord Coord = FCoord(x, y);
		if (i > 1) Difficulty = 1;
		if (i > 4) Difficulty = 2;
		if (i == NumRooms - 2) { ChanceOfGoingRight = 100; }
		if (i == NumRooms - 1) { Difficulty = 9; }
		// if (i == TreasureRoomPosition) TreasureRoomCoord = Coord;
		FRoomState Content; 
		// first room will always be empty
		if (i == 0) Content.RoomType = GetRandomRoom(Difficulty, ERoomType::Empty);
		else
		{
			// all next rooms will always have regular enemies, unless it's the last room, then it's a boss room
			if (i == NumRooms - 1) { Content.RoomType = GetRandomRoom(Difficulty, ERoomType::Boss); }
			else Content.RoomType = GetRandomRoom(Difficulty, ERoomType::Enemies);
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
				if (IsNeighborFree(Coord, EWallPos::Top)) x++;
				else x--;
			}
			else
			{
				if (IsNeighborFree(Coord, EWallPos::Bottom)) x--;
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

/* Spawns one room at Where Coords using RoomType DataAsset. Will load the streaming level for
that DataAsset, load it and set it visible to spawn it in the world. Returns a pointer to the
Streaming Level that it spawned. */
void ALevelBuilder::SpawnLevels()
{
	FTransform RoomLoc = FTransform();
	for (auto &&Tile : Grid)
	{
		ULevelStreaming* NewRoom = SpawnRoom(Tile.Key, Tile.Value.RoomType);
		// UE_LOG(LogTemp, Warning, TEXT("created %s room at %s"), *Tile.Value.RoomType->LevelAddress.ToString(), *Tile.Key.ToString());
		BuildWalls(Tile);
	}
	
	// for (auto &&Wall : AllWalls)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Wall: %s"), *Wall.Key);
	// }
	
}
ULevelStreaming* ALevelBuilder::SpawnRoom(FCoord Where, class URoomDataAsset* RoomType)
{
	ULevelStreaming* NewRoom = OnBPCreateLevelByName(RoomType->GetAutoLevelAddress());
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

// Try to Create a Wall at this Grid Coord to direction Dir. Will not create the wall if it already exists
// Creates a WallSettings and set its Doored type and calls TryToGenerateWallAtGrid with that Settings
AWall* ALevelBuilder::GenerateWallOfDoorTypeAtGrid(FCoord Where, EWallPos Dir, bool Doored = false)
{
	AWall* result = nullptr;
	FWallSettings NewSettings = FWallSettings();
	FWallSettings* Settings = &NewSettings;
	Settings->bIsDoored = Doored;
	if (Doored)
	{
		result = TryGenerateWallAtGrid(Where, Dir, Settings);
		if (result)
		{
			SpawnDoor(Where, Dir);
		}
	}
	else 
	{
		result = TryGenerateWallAtGrid(Where, Dir, Settings);
	}
	return result;
}
// Try to Create a Wall at this Grid Coord to direction Dir. Will not create the wall if it already exists
AWall* ALevelBuilder::TryGenerateWallAtGrid(FCoord Where, EWallPos Dir, FWallSettings* Settings)
{
	FString ID = GetWallID(Where, Dir);
	AWall** Existing = AllWalls.Find(ID);
	if (Existing) {
		// UE_LOG(LogTemp, Warning, TEXT("Wall of ID: %s, already exists"), *ID);
		return nullptr;
	}
	FTransform RoomLoc = FTransform();
	RoomLoc.SetLocation(GetLocFromGrid(Where));
	AWall* NewWall = GenerateWallAtLoc(RoomLoc, Dir, Settings);
	AllWalls.Add(ID, NewWall);
	// UE_LOG(LogTemp, Warning, TEXT("Generated wall of ID: %s, total %d walls"), *ID, AllWalls.Num());
	return NewWall;
}

AWall* ALevelBuilder::GenerateEdgeWallAtGrid(FCoord Where, EWallPos Pos)
{
	FCoord SideCoord = GetNeighbor(Where, Pos);
	FRoomState* Side = Grid.Find(SideCoord);
	if (!Side)
	{
		GenerateWallOfDoorTypeAtGrid(Where, Pos);
	}
	return nullptr;
}

/* Spawns a wall in the room centered in Transform Where, the wall will be at Direction Pos of that room
Will use given Wall Settings but will edit the Settings length depending on Room Size */
AWall* ALevelBuilder::GenerateWallAtLoc(FTransform Where, EWallPos Pos, FWallSettings* Settings)
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
	case EWallPos::Top:
		Loc.X += RoomSizeX/2;
		Settings->Length = RoomSizeY;
		break;

	case EWallPos::Bottom:
		Loc.X -= RoomSizeX/2;
		Settings->Length = RoomSizeY;
		break;
	
	case EWallPos::Left:
		Loc.Y -= RoomSizeY/2;
		Rot.Yaw = 90.0f;
		Settings->Length = RoomSizeX;
		break;
	
	case EWallPos::Right:
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

ADoor* ALevelBuilder::SpawnDoor(FCoord Where, EWallPos Dir)
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
URoomDataAsset* ALevelBuilder::GetRandomRoom(int32 Difficulty, ERoomType Type)
{
	if (RoomList.Num() == 0) return nullptr;
	TArray<URoomDataAsset*> FilteredRooms = {};
	FilteredRooms = FindRoomsOfType(Type, Difficulty);
	if (FilteredRooms.Num() == 0) return nullptr;
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
*/
TArray<URoomDataAsset*> ALevelBuilder::FindRoomsOfType(ERoomType Type, int32 Difficulty)
{
	TArray<URoomDataAsset *> FilteredRooms = {};
	if (RoomList.Num() == 0) return FilteredRooms;
	for (auto &&Room : RoomList)
	{
		if (Type == Room->RoomType && (Difficulty < 0 || Room->RoomDifficulty == Difficulty))
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
		UE_LOG(LogTemp, Warning, TEXT("Grid Coord: %s, Room: %s"), *Tile.Key.ToString(), *Tile.Value.RoomType->LevelAddress.ToString());
		FString line = FString::Printf(TEXT("Coord: %s, Room: %s"), *Tile.Key.ToString(), *Tile.Value.RoomType->LevelAddress.ToString());
		result.Append(line);
	}
	return result;
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
			GenerateWallOfDoorTypeAtGrid(Tile.Key, Dir, true);
		}
	}
	
}

FVector ALevelBuilder::GetLocFromGrid(FCoord Coord)
{
	return FVector(RoomSizeX * (float)Coord.X, RoomSizeY * (float)Coord.Y, 0.0f);
}

FTransform ALevelBuilder::GetWallLocFromGridAndDir(FCoord Coord, EWallPos Dir)
{
	FVector Loc = GetLocFromGrid(Coord);
	FRotator Rot = FRotator::ZeroRotator;
	switch (Dir)
	{
	case EWallPos::Top:
		Loc.X += RoomSizeX/2;
		break;

	case EWallPos::Bottom:
		Loc.X -= RoomSizeX/2;
		break;
	
	case EWallPos::Left:
		Loc.Y -= RoomSizeY/2;
		Rot.Yaw = 90.0f;
		break;
	
	case EWallPos::Right:
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

/* Returns the Grid Coord of the Neighbor of this Coord From to direction To
 */
FCoord ALevelBuilder::GetNeighbor(FCoord From, EWallPos To)
{
	return FCoord(From.X + GetXFromDir(To), From.Y + GetYFromDir(To));
}

/* Returns true if the neighbor From this Coord To this direction does not have any 
Rooms in the Grid yet
 */
bool ALevelBuilder::IsNeighborFree(FCoord From, EWallPos To)
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
		if (!NeighborState) continue;
		if (NeighborState->RoomType->RoomType == Type) return true;
	}
	return false;
}
/* Returns true if this Tile at this Coord is of given Room Type */
bool ALevelBuilder::IsTileOfType(FCoord Tile, ERoomType Type)
{
	FRoomState* MyState = Grid.Find(Tile);
	if (!MyState) false;
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

FRoomState* ALevelBuilder::GetRoomStateFromCoord(FCoord Coord)
{
	// if (!Grid) return nullptr;
	FRoomState* GS = Grid.Find(Coord);
	return GS;
}

void ALevelBuilder::OnEnterRoom(FRoomState NewRoom)
{
	if (NewRoom.RoomType->RoomType == ERoomType::Boss && BossMusic)
	{
		if (LevelMusic) LevelMusicRef->Stop();
		UGameplayStatics::PlaySound2D(GetWorld(), BossMusic);
	}

	OnEnterRoomDelegate.Broadcast(NewRoom);
}

URoomDataAsset* ALevelBuilder::GetRoomFromCoord(FCoord Coord)
{
	FRoomState* GS = Grid.Find(Coord);
	return GS->RoomType;
}

AWall* ALevelBuilder::GetBottomWallFromLoc(FVector Location)
{
	return GetWallRefFromCoordAndDir(GetGridFromLoc(Location), EWallPos::Bottom);
}
	
AWall* ALevelBuilder::GetWallRefFromCoordAndDir(FCoord Coord, EWallPos Dir)
{
	// UE_LOG(LogTemp, Warning, TEXT("Requested wall on %s"), *Coord.ToString());
	FString ID = GetWallID(Coord, Dir);
	AWall** Wall = AllWalls.Find(ID);
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
	UE_LOG(LogTemp, Warning, TEXT("Char at location %s, which is coord %s"), *Location.ToString(), *Coord.ToString());
	FRoomState* Room = GetRoomStateFromCoord(Coord);
	OnEnterRoom(*Room);
	UE_LOG(LogTemp, Warning, TEXT("Room? %d, Room: %s, isDoored: %d"), (Room != nullptr), *Room->RoomType->LevelAddress.ToString(), Room->RoomType->bIsDoored);
	if ((Room != nullptr) && !Room->bIsRoomCleared && Room->RoomType->bIsDoored)
	{
		UE_LOG(LogTemp, Warning, TEXT("closing doors"));
		TeleportPlayerInsideRoom(Location);
		//DebugGrid();
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
		if (Wall)
		{
			Wall->Height = 400.f;
			Wall->BuildWalls();
		}
	}
	HiddenWalls = {};
	AWall* ThisWall = GetWallRefFromCoordAndDir(Coord, Dir);
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



