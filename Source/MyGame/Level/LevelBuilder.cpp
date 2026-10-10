// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBuilder.h"
#include "Door.h"
#include "Wall.h"
#include "RoomMaster.h"
#include "../MyGameInstance.h"

#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Math/RandomStream.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

TArray<ERoomDirection> ALLDIRECTIONS = {ERoomDirection::Left, ERoomDirection::Right, ERoomDirection::Bottom, ERoomDirection::Top};
int8 GetXFromDir(ERoomDirection Dir)
{
	if (Dir == ERoomDirection::Top) return 1;
	if (Dir == ERoomDirection::Bottom) return -1;
	return 0;
}
int8 GetYFromDir(ERoomDirection Dir)
{
	if (Dir == ERoomDirection::Right) return 1;
	if (Dir == ERoomDirection::Left) return -1;
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

	GetWorldSettings()->SetTimeDilation(0.0f);
	// RandomStream = FRandomStream(RandomSeed);
	// if (!RandomSeed) RandomStream.GenerateNewSeed();

	if (UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance())) MyGI->SetLevelBuilderRef(this);
	LoadRoomCatalogue();
	BuildGrid();
	SpawnLevels();
	// The game stays frozen until every room has streamed in: unfreeze when nothing streams, and don't wait forever
	if (CountOfLevelsThatDidntFinishLoading == 0) GetWorldSettings()->SetTimeDilation(1.0f);
	else LoadingTimeout = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		OnLoadingTimedOut();
		return false;
	}), 15.f);

	if (LevelMusic) LevelMusicRef = UGameplayStatics::SpawnSound2D(GetWorld(), LevelMusic);
	UE_LOG(LogTemp, Warning, TEXT("Grid: %s"), *DebugGrid());
	// GenerateRoom();
}

void ALevelBuilder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTSTicker::GetCoreTicker().RemoveTicker(LoadingTimeout);
	Super::EndPlay(EndPlayReason);
}

/* Plans the run's rooms (FLevelLayoutGenerator) and fills the Grid with them */
void ALevelBuilder::BuildGrid()
{
	UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
	if (!GI) return;
	RandomStream = &GI->RandomStream;
	FLevelLayoutParams Params;
	Params.NumRooms = NumRooms + GI->LevelDifficulty;
	Params.InitialChanceOfGoingRight = InitialChanceOfGoingRight;
	Params.DecWhenChoseRight = DecWhenChoseRight;
	Params.IncWhenChoseVert = IncWhenChoseVert;
	Layout = FLevelLayoutGenerator::Generate(Params, ObjectPtrDecay(RoomList), *RandomStream);
	for (const FPlannedRoom& Planned : Layout.Rooms)
	{
		FRoomState Content;
		Content.RoomType = Planned.Room;
		if (Planned.bMinibossReward) Content.RoomReward = MinibossReward;
		Grid.Add(FCoord(Planned.Coord.X, Planned.Coord.Y), Content);
	}
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
		BuildWalls(Tile);
		FString RoomAddress = "invalid";
		if (Tile.Value.RoomType) RoomAddress = Tile.Value.RoomType->Level.GetAssetName();
		FString line = FString::Printf(TEXT("Coord: %s, Room: %s, Walls: %d"), *Tile.Key.ToString(), *RoomAddress, Tile.Value.Walls.Num());
		UE_LOG(LogTemp, Warning, TEXT("%s"), *line);
	}
	
}

/* Streams one room's map in as a level instance at the Where grid cell. Returns its streaming level */
ULevelStreaming* ALevelBuilder::SpawnRoom(FCoord Where, class URoomDataAsset* RoomType)
{
	if (!RoomType || RoomType->Level.IsNull()) return nullptr;
	bool bSuccess = false;
	ULevelStreamingDynamic* NewRoom = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(this, RoomType->Level, GetLocFromGrid(Where), FRotator::ZeroRotator, bSuccess);
	if (!NewRoom)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not stream %s for %s"), *RoomType->Level.ToString(), *RoomType->GetName());
		return nullptr;
	}
	NewRoom->OnLevelLoaded.AddDynamic(this, &ALevelBuilder::OnLoadedOneLevel);
	CountOfLevelsThatDidntFinishLoading++;
	return NewRoom;
}

// A room stalled: start rather than stay frozen
void ALevelBuilder::OnLoadingTimedOut()
{
	UE_LOG(LogTemp, Error, TEXT("%d rooms didn't finish streaming in after 15 s; starting anyway"), CountOfLevelsThatDidntFinishLoading);
	GetWorldSettings()->SetTimeDilation(1.0f);
}

// Bound to each streamed room's OnLevelLoaded; unfreezes the game when the last one is in
void ALevelBuilder::OnLoadedOneLevel()
{
	CountOfLevelsThatDidntFinishLoading--;
	if (CountOfLevelsThatDidntFinishLoading == 0)
	{
		FTSTicker::GetCoreTicker().RemoveTicker(LoadingTimeout);
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
AWall* ALevelBuilder::TrySpawnWallCoordDir(TPair<FCoord, FRoomState> &Tile, ERoomDirection Dir, bool Doored = false)
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
AWall* ALevelBuilder::TrySpawnWallFromSettings(TPair<FCoord, FRoomState>& Tile, ERoomDirection Dir, FWallSettings* Settings)
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
		ERoomDirection OppositeDir = GetOppositeDirection(Dir);
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
AWall* ALevelBuilder::TrySpawnEdgeWallAtCoord(TPair<FCoord, FRoomState> &Tile, ERoomDirection Pos)
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
AWall* ALevelBuilder::SpawnWallAtLocDirSettings(FTransform Where, ERoomDirection Pos, FWallSettings* Settings)
{
	FVector Loc = Where.GetLocation();
	FRotator Rot = Where.Rotator();
	FWallSettings DefaultSettings;
	if (!Settings) Settings = &DefaultSettings;
	switch (Pos)
	{
	case ERoomDirection::Top:
		Loc.X += RoomSizeX/2;
		Settings->Length = RoomSizeY;
		break;

	case ERoomDirection::Bottom:
		Loc.X -= RoomSizeX/2;
		Settings->Length = RoomSizeY;
		break;
	
	case ERoomDirection::Left:
		Loc.Y -= RoomSizeY/2;
		Rot.Yaw = 90.0f;
		Settings->Length = RoomSizeX;
		break;
	
	case ERoomDirection::Right:
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
	FWallSettings DefaultSettings;
	if (!Settings) Settings = &DefaultSettings;
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

ADoor* ALevelBuilder::SpawnDoor(FCoord Where, ERoomDirection Dir)
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

void ALevelBuilder::LoadRoomCatalogue()
{
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetId> RoomIds;
	AssetManager.GetPrimaryAssetIdList(URoomDataAsset::StaticClass()->GetFName(), RoomIds);
	RoomIds.Sort([](const FPrimaryAssetId& A, const FPrimaryAssetId& B) { return A.PrimaryAssetName.LexicalLess(B.PrimaryAssetName); });
	RoomList.Reset();
	for (const FPrimaryAssetId& Id : RoomIds)
	{
		if (URoomDataAsset* Room = Cast<URoomDataAsset>(AssetManager.GetPrimaryAssetPath(Id).TryLoad())) RoomList.Add(Room);
		else UE_LOG(LogTemp, Error, TEXT("Room %s didn't load"), *Id.ToString());
	}
}

FString ALevelBuilder::DebugGrid()
{
	FString result = FString();
	for (auto&& Tile : Grid)
	{
		FString RoomAddress = "invalid";
		if (Tile.Value.RoomType) RoomAddress = Tile.Value.RoomType->Level.GetAssetName();
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

FTransform ALevelBuilder::GetWallLocFromGridAndDir(FCoord Coord, ERoomDirection Dir)
{
	FVector Loc = GetLocFromGrid(Coord);
	FRotator Rot = FRotator::ZeroRotator;
	switch (Dir)
	{
	case ERoomDirection::Top:
		Loc.X += RoomSizeX/2;
		break;

	case ERoomDirection::Bottom:
		Loc.X -= RoomSizeX/2;
		break;
	
	case ERoomDirection::Left:
		Loc.Y -= RoomSizeY/2;
		Rot.Yaw = 90.0f;
		break;
	
	case ERoomDirection::Right:
		Loc.Y += RoomSizeY/2;
		Rot.Yaw = 90.0f;
		break;
	}
	FTransform result = FTransform(Rot, Loc);
	return result;
}

FCoord ALevelBuilder::GetGridFromLoc(FVector Location)
{
	int16 NewX = FMath::RoundToInt(Location.X / RoomSizeX);
	int16 NewY = FMath::RoundToInt(Location.Y / RoomSizeY);
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
}

FString ALevelBuilder::GetWallID(FCoord Coord, ERoomDirection Dir)
{
	return GetWallID(Coord, GetNeighbor(Coord, Dir));
}

/* Returns the Grid Coord of the Neighbor of this Coord From to direction To
 */
FCoord ALevelBuilder::GetNeighbor(FCoord From, ERoomDirection To)
{
	return FCoord(From.X + GetXFromDir(To), From.Y + GetYFromDir(To));
}

/* Returns true if this Tile at this Coord is of given Room Type */
bool ALevelBuilder::IsTileOfType(FCoord Tile, ERoomType Type)
{
	FRoomState* MyState = Grid.Find(Tile);
	if (!MyState || !MyState->RoomType) return false;
	if (MyState->RoomType->RoomType == Type) return true;
	return false;
}

/// <summary>
/// Returns what the ERoomDirection is the opposite of From. For example, Left is opposite of Right.
/// Used by wall spawning to figure what direction to add wall data to both tiles the wall is between.
/// </summary>
/// <param name="From">Direction to get the opposite from</param>
/// <returns>The opposite direction of From</returns>
ERoomDirection ALevelBuilder::GetOppositeDirection(ERoomDirection From)
{
	switch (From)
	{
	case ERoomDirection::Left:
		return ERoomDirection::Right;
	case ERoomDirection::Right:
		return ERoomDirection::Left;
	case ERoomDirection::Bottom:
		return ERoomDirection::Top;
	case ERoomDirection::Top:
		return ERoomDirection::Bottom;
	default:
		break;
	}
	return ERoomDirection();
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
	if (!Room || !NewRoomMaster)
	{
		UE_LOG(LogTemp, Error, TEXT("RegisterRoomMaster: no room in the grid at %s"), *Location.ToString());
		return;
	}
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
	return GetWallRefFromCoordAndDir(GetGridFromLoc(Location), ERoomDirection::Bottom);
}
	
AWall* ALevelBuilder::GetWallRefFromCoordAndDir(FCoord Coord, ERoomDirection Dir)
{
	// UE_LOG(LogTemp, Warning, TEXT("Requested wall on %s"), *Coord.ToString());
	FString ID = GetWallID(Coord, Dir);
	AWall** Wall = AllWalls.Find(ID);
	if (Wall) return *Wall;
	return nullptr;
}

// Try to hide walls on the Grid at this Location
// Called from the Character Every tick with the Actor location
void ALevelBuilder::OnUpdateCharCoord(FVector Location, ERoomDirection Dir)
{
	FCoord Coord = GetGridFromLoc(Location);
	if (Coord == LastEnteredRoomCoord) return;
	LastEnteredRoomCoord = Coord;
	UE_LOG(LogTemp, Warning, TEXT("Char at location %s, which is coord %s"), *Location.ToString(), *Coord.ToString());
	HideWall(Coord, Dir);
	FRoomState* Room = GetRoomStateFromCoord(Coord);
	// this might actually be null if I dash into a pit UNDER the wall like an idiot
	if (!Room) return;
	// TODO pass location as parameter
	OnEnterRoom(*Room);
	ARoomMaster* Master = Room->RoomMasterRef;
	// TODO this fails on coord 0,0 because game starts before room master spawns
	if (!Master) return;
	if (!Room->bIsRoomCleared && Room->RoomType && Room->RoomType->bIsDoored && !Master->AreAllCharsDead())
	{
		//UE_LOG(LogTemp, Warning, TEXT("closing doors"));
		TeleportPlayerInsideRoom(Location);
		//DebugGrid();
		if (Master) Master->bIsDoorOpen = false;
		CloseDoors();
	}
}

// Try to Hide walls on this Grid Tile
void ALevelBuilder::HideWall(FCoord Coord, ERoomDirection Dir)
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
	FCoord RightCoord = GetNeighbor(Coord, ERoomDirection::Right);
	AWall* RightWall = GetWallRefFromCoordAndDir(RightCoord, Dir);
	URoomDataAsset* RightRoom = GetRoomFromCoord(RightCoord);
	// if room to the right is not doored, then hide it too
	if (RightRoom && !RightRoom->bIsDoored)	HideOneWall(RightWall);
	FCoord LeftCoord = GetNeighbor(Coord, ERoomDirection::Left);
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



