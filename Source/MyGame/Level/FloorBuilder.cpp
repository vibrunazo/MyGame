// Fill out your copyright notice in the Description page of Project Settings.


#include "FloorBuilder.h"
#include "Door.h"
#include "Wall.h"
#include "TileMaster.h"
#include "RunSubsystem.h"
#include "TileDressing.h"
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

TArray<ETileSide> ALLDIRECTIONS = {ETileSide::Left, ETileSide::Right, ETileSide::Bottom, ETileSide::Top};
int8 GetXFromDir(ETileSide Dir)
{
	if (Dir == ETileSide::Top) return 1;
	if (Dir == ETileSide::Bottom) return -1;
	return 0;
}
int8 GetYFromDir(ETileSide Dir)
{
	if (Dir == ETileSide::Right) return 1;
	if (Dir == ETileSide::Left) return -1;
	return 0;
}


// Sets default values
AFloorBuilder::AFloorBuilder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// PrimaryActorTick.bCanEverTick = true;
	BBComp = CreateDefaultSubobject<UBillboardComponent>(TEXT("Dino"));
	RootComponent = BBComp;
	TileBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	// TileBounds->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	
	if (!WallActor) WallActor = AWall::StaticClass();
	// BBComp->SetupAttachment(RootComponent);

}

void AFloorBuilder::OnConstruction(const FTransform & Transform)
{
	TileBounds->SetBoxExtent(FVector(TileSizeX/2, TileSizeY/2, 100.f));
	TileBounds->SetWorldLocation(FVector(0.f, 0.f, -100.f));
}

// Called when the game starts or when spawned
void AFloorBuilder::BeginPlay()
{
	Super::BeginPlay();

	GetWorldSettings()->SetTimeDilation(0.0f);
	// RandomStream = FRandomStream(RandomSeed);
	// if (!RandomSeed) RandomStream.GenerateNewSeed();

	GetWorld()->GetSubsystem<URunSubsystem>()->SetFloorBuilder(this);
	LoadTileTemplates();
	PlanTiles();
	SpawnLevels();
	FindRooms();
	// The game stays frozen until every room has streamed in: unfreeze when nothing streams, and don't wait forever
	if (CountOfLevelsThatDidntFinishLoading == 0) OnTilesReady();
	else LoadingTimeout = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		OnLoadingTimedOut();
		return false;
	}), 15.f);

	if (LevelMusic) LevelMusicRef = UGameplayStatics::SpawnSound2D(GetWorld(), LevelMusic);
	UE_LOG(LogTemp, Warning, TEXT("Tiles: %s"), *DebugTiles());
	// GenerateRoom();
}

void AFloorBuilder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTSTicker::GetCoreTicker().RemoveTicker(LoadingTimeout);
	Super::EndPlay(EndPlayReason);
}

/* Plans the run's rooms (FFloorLayoutGenerator) and fills the Tiles with them */
void AFloorBuilder::PlanTiles()
{
	UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
	if (!GI) return;
	RandomStream = &GI->RandomStream;
	FFloorLayoutParams Params;
	Params.NumTiles = NumTiles + GI->FloorNumber;
	Params.InitialChanceOfGoingRight = InitialChanceOfGoingRight;
	Params.DecWhenChoseRight = DecWhenChoseRight;
	Params.IncWhenChoseVert = IncWhenChoseVert;
	Layout = FFloorLayoutGenerator::Generate(Params, ObjectPtrDecay(TileTemplates), *RandomStream);
	for (const FPlannedTile& Planned : Layout.Tiles)
	{
		FTileState Content;
		Content.Template = Planned.Template;
		if (Planned.bMinibossReward) Content.Reward = MinibossReward;
		Tiles.Add(FTileCoord(Planned.Coord.X, Planned.Coord.Y), Content);
	}
}

/// <summary>
/// Spawns all rooms from the Tiles then spawns walls. Calls SpawnTile for each Room on the Tiles.
/// Called from BeginPlay after PlanTiles()
/// </summary>
void AFloorBuilder::SpawnLevels()
{
	FTransform RoomLoc = FTransform();
	for (auto &&Tile : Tiles)
	{
		ULevelStreaming* NewRoom = SpawnTile(Tile.Key, Tile.Value.Template);
		Tile.Value.LevelRef = NewRoom;
		BuildWalls(Tile);
		FString TemplateName = "invalid";
		if (Tile.Value.Template) TemplateName = Tile.Value.Template->Level.GetAssetName();
		FString line = FString::Printf(TEXT("Coord: %s, Room: %s, Walls: %d"), *Tile.Key.ToString(), *TemplateName, Tile.Value.Walls.Num());
		UE_LOG(LogTemp, Warning, TEXT("%s"), *line);
	}
	
}

/* Streams one room's map in as a level instance at the Where grid cell. Returns its streaming level */
ULevelStreaming* AFloorBuilder::SpawnTile(FTileCoord Where, class UTileTemplate* Template)
{
	if (!Template || Template->Level.IsNull()) return nullptr;
	bool bSuccess = false;
	ULevelStreamingDynamic* NewRoom = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(this, Template->Level, GetLocFromTile(Where), FRotator::ZeroRotator, bSuccess);
	if (!NewRoom)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not stream %s for %s"), *Template->Level.ToString(), *Template->GetName());
		return nullptr;
	}
	// shown, not just loaded: the room's floors must collide before props are dropped on them
	NewRoom->OnLevelShown.AddDynamic(this, &AFloorBuilder::OnLoadedOneLevel);
	CountOfLevelsThatDidntFinishLoading++;
	return NewRoom;
}

// A room stalled: start rather than stay frozen
void AFloorBuilder::OnLoadingTimedOut()
{
	UE_LOG(LogTemp, Error, TEXT("%d rooms didn't finish streaming in after 15 s; starting anyway"), CountOfLevelsThatDidntFinishLoading);
	OnTilesReady();
}

void AFloorBuilder::OnTilesReady()
{
	if (bTilesReady) return;
	bTilesReady = true;
	GetWorldSettings()->SetTimeDilation(1.0f);
	for (auto&& Tile : Tiles) DressTile(Tile.Key, Tile.Value);
}

// Props go along the room's solid walls, except the bottom one (the camera's side), and never in a doorway or on an
// open edge into the next room. The seed comes from the run seed and the room, so a run always gets the same props.
void AFloorBuilder::DressTile(const FTileCoord& Coord, const FTileState& Room)
{
	if (!DressingGraph) return;
	const FVector Center = GetLocFromTile(Coord) + FVector(0.f, 0.f, 500.f);
	const float HX = TileSizeX / 2 - DressingInset, HY = TileSizeY / 2 - DressingInset;
	// deferred: the PCG component reads the actor's bounds when it registers, so size the room first
	ATileDressing* Dressing = GetWorld()->SpawnActorDeferred<ATileDressing>(ATileDressing::StaticClass(), FTransform(Center));
	Dressing->SetTileExtent(FVector(TileSizeX / 2, TileSizeY / 2, 1000.f));
	Dressing->FinishSpawning(FTransform(Center));
	auto AddWallLine = [&](ETileSide Dir, const FVector& From, const FVector& To)
	{
		if (!Room.Walls.Contains(Dir)) return;
		const AWall* Wall = GetWallAtTileSide(Coord, Dir);
		if (!Wall || !Wall->bIsDoored)
		{
			Dressing->AddLine(From, To);
			return;
		}
		const FVector Mid = (From + To) / 2, Along = (To - From).GetSafeNormal();
		Dressing->AddLine(From, Mid - Along * DoorwayClearance);
		Dressing->AddLine(Mid + Along * DoorwayClearance, To);
	};
	AddWallLine(ETileSide::Top, Center + FVector(HX, -HY, 0.f), Center + FVector(HX, HY, 0.f));
	AddWallLine(ETileSide::Left, Center + FVector(-HX, -HY, 0.f), Center + FVector(HX, -HY, 0.f));
	AddWallLine(ETileSide::Right, Center + FVector(-HX, HY, 0.f), Center + FVector(HX, HY, 0.f));
	Dressing->Generate(DressingGraph, (int32)HashCombine(GetTypeHash(RandomStream ? RandomStream->GetInitialSeed() : 0), GetTypeHash(Coord)));
	Dressings.Add(Dressing);
}

// Bound to each streamed room's OnLevelShown; unfreezes the game when the last one is in
void AFloorBuilder::OnLoadedOneLevel()
{
	CountOfLevelsThatDidntFinishLoading--;
	if (CountOfLevelsThatDidntFinishLoading == 0)
	{
		FTSTicker::GetCoreTicker().RemoveTicker(LoadingTimeout);
		OnTilesReady();
	}
	// UE_LOG(LogTemp, Warning, TEXT("new level is loaded, %d levels left to load"), CountOfLevelsThatDidntFinishLoading);
}

/// <summary>
/// Spawns the walls for one Tile of the Tiles.
/// Called by SpawnLevels()
/// </summary>
/// <param name="Tile">One Tile of the Tiles Map, contains the Coord the Room is at and the State of the Room</param>
void AFloorBuilder::BuildWalls(TPair<FTileCoord, FTileState> &Tile)
{
	// edge of world walls
	for (auto&& Dir : ALLDIRECTIONS)
	{
		auto NewWall = TrySpawnEdgeWallAtCoord(Tile, Dir);
	}
	// room type walls
	if (Tile.Value.Template && Tile.Value.Template->bIsWalled)
	{
		for (auto&& Dir : ALLDIRECTIONS)
		{
			// GenerateWallAtGrid(Tile.Key, Dir, WallDooredMesh);
			auto NewWall = TrySpawnWallCoordDir(Tile, Dir, true);
		}
	}
}

// Try to Spawn a Wall at this Tile on the Tiles in direction Dir. Will not create the wall if it already exists
// Creates a WallSettings and set its Doored type and calls TryToGenerateWallAtGrid with that Settings
AWall* AFloorBuilder::TrySpawnWallCoordDir(TPair<FTileCoord, FTileState> &Tile, ETileSide Dir, bool Doored = false)
{
	FTileCoord Where = Tile.Key;
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
// Try to Create a Wall at this Tiles Coord to direction Dir. Will not create the wall if it already exists
AWall* AFloorBuilder::TrySpawnWallFromSettings(TPair<FTileCoord, FTileState>& Tile, ETileSide Dir, FWallSettings* Settings)
{
	FTileCoord Where = Tile.Key;
	FString ID = GetWallID(Where, Dir);
	AWall** Existing = AllWalls.Find(ID);
	if (Existing) {
		// UE_LOG(LogTemp, Warning, TEXT("Wall of ID: %s, already exists"), *ID);
		return nullptr;
	}
	FTransform RoomLoc = FTransform();
	RoomLoc.SetLocation(GetLocFromTile(Where));
	AWall* NewWall = SpawnWallAtLocDirSettings(RoomLoc, Dir, Settings);
	if (NewWall)
	{
		AllWalls.Add(ID, NewWall);
		//UE_LOG(LogTemp, Warning, TEXT("Adding wall at %s, Dir: %d"), *Where.ToString(), Dir);
		if (!Tile.Value.Walls.Contains(Dir)) Tile.Value.Walls.Add(Dir);
		FTileState* Neighbor = Tiles.Find(GetNeighbor(Where, Dir));
		ETileSide OppositeDir = GetOppositeDirection(Dir);
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
AWall* AFloorBuilder::TrySpawnEdgeWallAtCoord(TPair<FTileCoord, FTileState> &Tile, ETileSide Pos)
{
	FTileCoord Where = Tile.Key;
	FTileCoord SideCoord = GetNeighbor(Where, Pos);
	FTileState* Side = Tiles.Find(SideCoord);
	if (!Side)
	{
		AWall* NewWall = TrySpawnWallCoordDir(Tile, Pos);
		if (NewWall) return NewWall;
	}
	return nullptr;
}

/* Spawns a wall in the room centered in Transform Where, the wall will be at Direction Pos of that room
Will use given Wall Settings but will edit the Settings length depending on Room Size */
AWall* AFloorBuilder::SpawnWallAtLocDirSettings(FTransform Where, ETileSide Pos, FWallSettings* Settings)
{
	FVector Loc = Where.GetLocation();
	FRotator Rot = Where.Rotator();
	FWallSettings DefaultSettings;
	if (!Settings) Settings = &DefaultSettings;
	switch (Pos)
	{
	case ETileSide::Top:
		Loc.X += TileSizeX/2;
		Settings->Length = TileSizeY;
		break;

	case ETileSide::Bottom:
		Loc.X -= TileSizeX/2;
		Settings->Length = TileSizeY;
		break;
	
	case ETileSide::Left:
		Loc.Y -= TileSizeY/2;
		Rot.Yaw = 90.0f;
		Settings->Length = TileSizeX;
		break;
	
	case ETileSide::Right:
		Loc.Y += TileSizeY/2;
		Rot.Yaw = 90.0f;
		Settings->Length = TileSizeX;
		break;
	}
	return SpawnWall(FTransform(Rot, Loc), Settings);
}

/* Spawns a new AWall exactly at World Transform Where with given Wall Settings */
AWall* AFloorBuilder::SpawnWall(FTransform Where, FWallSettings* Settings)
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

ADoor* AFloorBuilder::SpawnDoor(FTileCoord Where, ETileSide Dir)
{
	FTransform DoorTran = GetWallLocFromTileAndSide(Where, Dir);
	FVector Loc = DoorTran.GetLocation();
	FActorSpawnParameters params;
	params.bNoFail = true;
	ADoor* NewDoor = GetWorld()->SpawnActor<ADoor>(DoorActor, Loc, DoorTran.Rotator(), params);
	NewDoor->OpenDoor();
	DoorList.Add(NewDoor);
	if (IsTileOfType(Where, ETileType::Treasure) || IsTileOfType(GetNeighbor(Where, Dir), ETileType::Treasure))
	{
		NewDoor->SetSymbol(ETileType::Treasure);
	}
	if (IsTileOfType(Where, ETileType::Boss) || IsTileOfType(GetNeighbor(Where, Dir), ETileType::Boss))
	{
		NewDoor->SetSymbol(ETileType::Boss);
	}
	return NewDoor;
}

void AFloorBuilder::LoadTileTemplates()
{
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetId> TemplateIds;
	AssetManager.GetPrimaryAssetIdList(UTileTemplate::StaticClass()->GetFName(), TemplateIds);
	TemplateIds.Sort([](const FPrimaryAssetId& A, const FPrimaryAssetId& B) { return A.PrimaryAssetName.LexicalLess(B.PrimaryAssetName); });
	TileTemplates.Reset();
	for (const FPrimaryAssetId& Id : TemplateIds)
	{
		if (UTileTemplate* Room = Cast<UTileTemplate>(AssetManager.GetPrimaryAssetPath(Id).TryLoad())) TileTemplates.Add(Room);
		else UE_LOG(LogTemp, Error, TEXT("Room %s didn't load"), *Id.ToString());
	}
}

FString AFloorBuilder::DebugTiles()
{
	FString result = FString();
	for (auto&& Tile : Tiles)
	{
		FString TemplateName = "invalid";
		if (Tile.Value.Template) TemplateName = Tile.Value.Template->Level.GetAssetName();
		FString line = FString::Printf(TEXT("Coord: %s, Room: %s, Walls: %d"), *Tile.Key.ToString(), *TemplateName, Tile.Value.Walls.Num());
		UE_LOG(LogTemp, Warning, TEXT("%s"), *line);
		result.Append(line);
	}
	return result;
}
FVector AFloorBuilder::GetLocFromTile(FTileCoord Coord)
{
	return FVector(TileSizeX * (float)Coord.X, TileSizeY * (float)Coord.Y, 0.0f);
}

FTransform AFloorBuilder::GetWallLocFromTileAndSide(FTileCoord Coord, ETileSide Dir)
{
	FVector Loc = GetLocFromTile(Coord);
	FRotator Rot = FRotator::ZeroRotator;
	switch (Dir)
	{
	case ETileSide::Top:
		Loc.X += TileSizeX/2;
		break;

	case ETileSide::Bottom:
		Loc.X -= TileSizeX/2;
		break;
	
	case ETileSide::Left:
		Loc.Y -= TileSizeY/2;
		Rot.Yaw = 90.0f;
		break;
	
	case ETileSide::Right:
		Loc.Y += TileSizeY/2;
		Rot.Yaw = 90.0f;
		break;
	}
	FTransform result = FTransform(Rot, Loc);
	return result;
}

FTileCoord AFloorBuilder::GetTileFromLoc(FVector Location)
{
	int16 NewX = FMath::RoundToInt(Location.X / TileSizeX);
	int16 NewY = FMath::RoundToInt(Location.Y / TileSizeY);
	return FTileCoord(NewX, NewY);
}

/// <summary>
/// From a given CurrentLocation returns the center of the Room that CurrentLocation is in
/// </summary>
/// <param name="CurrentLocation"></param>
/// <returns></returns>
FVector AFloorBuilder::GetTileCenter(FVector CurrentLocation)
{
	FTileCoord Coord = GetTileFromLoc(CurrentLocation);
	return GetLocFromTile(Coord);
}

FString AFloorBuilder::GetWallID(FTileCoord Coord1, FTileCoord Coord2)
{
	FString s1 = Coord1.ToString();
	FString s2 = Coord2.ToString();
	if (s1 < s2) return s1 + 'x' + s2;
	else return s2 + 'x' + s1;
}

FString AFloorBuilder::GetWallID(FTileCoord Coord, ETileSide Dir)
{
	return GetWallID(Coord, GetNeighbor(Coord, Dir));
}

/* Returns the Tiles Coord of the Neighbor of this Coord From to direction To
 */
void AFloorBuilder::FindRooms()
{
	Rooms.Reset();
	RoomOfTile.Reset();
	for (auto&& Start : Tiles)
	{
		const FIntPoint StartTile(Start.Key.X, Start.Key.Y);
		if (RoomOfTile.Contains(StartTile)) continue;
		// flood fill through every side without a wall
		FFloorRoom& Room = Rooms.AddDefaulted_GetRef();
		const int32 Index = Rooms.Num() - 1;
		TArray<FTileCoord> Open = { Start.Key };
		RoomOfTile.Add(StartTile, Index);
		while (Open.Num())
		{
			const FTileCoord Tile = Open.Pop();
			Room.Tiles.Add(FIntPoint(Tile.X, Tile.Y));
			const FTileState& State = Tiles[Tile];
			for (ETileSide Side : ALLDIRECTIONS)
			{
				const FTileCoord Next = GetNeighbor(Tile, Side);
				const FIntPoint NextTile(Next.X, Next.Y);
				if (State.Walls.Contains(Side) || !Tiles.Contains(Next) || RoomOfTile.Contains(NextTile)) continue;
				RoomOfTile.Add(NextTile, Index);
				Open.Add(Next);
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("%d tiles make %d rooms"), Tiles.Num(), Rooms.Num());
}

int32 AFloorBuilder::GetRoomOfTile(FIntPoint Tile) const
{
	const int32* Room = RoomOfTile.Find(Tile);
	return Room ? *Room : -1;
}

const TArray<FIntPoint>& AFloorBuilder::GetRoomTiles(int32 Room) const
{
	static const TArray<FIntPoint> None;
	return Rooms.IsValidIndex(Room) ? Rooms[Room].Tiles : None;
}

bool AFloorBuilder::AreJoined(const FIntPoint& A, const FIntPoint& B) const
{
	const FIntPoint D = B - A;
	if (FMath::Abs(D.X) + FMath::Abs(D.Y) != 1) return false;
	const FTileState* State = Tiles.Find(FTileCoord(A.X, A.Y));
	if (!State || !Tiles.Contains(FTileCoord(B.X, B.Y))) return false;
	const ETileSide Side = D.X > 0 ? ETileSide::Top : D.X < 0 ? ETileSide::Bottom : D.Y > 0 ? ETileSide::Right : ETileSide::Left;
	return !State->Walls.Contains(Side);
}

void AFloorBuilder::GetTileExtent(FIntPoint& Min, FIntPoint& Max) const
{
	Min = FIntPoint(MAX_int32, MAX_int32);
	Max = FIntPoint(MIN_int32, MIN_int32);
	for (auto&& Tile : Tiles)
	{
		Min = FIntPoint(FMath::Min<int32>(Min.X, Tile.Key.X), FMath::Min<int32>(Min.Y, Tile.Key.Y));
		Max = FIntPoint(FMath::Max<int32>(Max.X, Tile.Key.X), FMath::Max<int32>(Max.Y, Tile.Key.Y));
	}
}

FTileCoord AFloorBuilder::GetNeighbor(FTileCoord From, ETileSide To)
{
	return FTileCoord(From.X + GetXFromDir(To), From.Y + GetYFromDir(To));
}

/* Returns true if this Tile at this Coord is of given Room Type */
bool AFloorBuilder::IsTileOfType(FTileCoord Tile, ETileType Type)
{
	FTileState* MyState = Tiles.Find(Tile);
	if (!MyState || !MyState->Template) return false;
	if (MyState->Template->TileType == Type) return true;
	return false;
}

/// <summary>
/// Returns what the ETileSide is the opposite of From. For example, Left is opposite of Right.
/// Used by wall spawning to figure what direction to add wall data to both tiles the wall is between.
/// </summary>
/// <param name="From">Direction to get the opposite from</param>
/// <returns>The opposite direction of From</returns>
ETileSide AFloorBuilder::GetOppositeDirection(ETileSide From)
{
	switch (From)
	{
	case ETileSide::Left:
		return ETileSide::Right;
	case ETileSide::Right:
		return ETileSide::Left;
	case ETileSide::Bottom:
		return ETileSide::Top;
	case ETileSide::Top:
		return ETileSide::Bottom;
	default:
		break;
	}
	return ETileSide();
}

FTileState* AFloorBuilder::GetTileAt(FTileCoord Coord)
{
	// if (!Tiles) return nullptr;
	FTileState* GS = Tiles.Find(Coord);
	return GS;
}

FTileState* AFloorBuilder::GetTileOfLevel(const ULevel* Level)
{
	if (!Level) return nullptr;
	for (auto&& Tile : Tiles)
	{
		if (Tile.Value.LevelRef && Tile.Value.LevelRef->GetLoadedLevel() == Level) return &Tile.Value;
	}
	return nullptr;
}

FTileState* AFloorBuilder::RegisterTileMaster(ATileMaster* NewTileMaster)
{
	FTileState* Room = NewTileMaster ? GetTileOfLevel(NewTileMaster->GetLevel()) : nullptr;
	if (!Room)
	{
		UE_LOG(LogTemp, Error, TEXT("RegisterTileMaster: %s isn't in a streamed room"), *GetNameSafe(NewTileMaster));
		return nullptr;
	}
	Room->TileMasterRef = NewTileMaster;
	return Room;
}

/// <summary>
/// Called when player enters a new room. Broadcasts RoomEnterDelegate.
/// Called by OnUpdateCharCoord if curent room is different from last.
/// </summary>
/// <param name="NewRoom">State of the Room player is currently at</param>
void AFloorBuilder::OnEnterTile(FTileState NewRoom)
{
	if (NewRoom.Template && NewRoom.Template->TileType == ETileType::Boss && BossMusic)
	{
		if (LevelMusic && LevelMusicRef) LevelMusicRef->Stop();
		if (BossMusic && !BossMusicRef) BossMusicRef = UGameplayStatics::SpawnSound2D(GetWorld(), BossMusic);
	}

	OnEnterTileDelegate.Broadcast(NewRoom);
}

UTileTemplate* AFloorBuilder::GetTemplateAt(FTileCoord Coord)
{
	FTileState* GS = Tiles.Find(Coord);
	if (GS) return GS->Template;
	return nullptr;
}

AWall* AFloorBuilder::GetBottomWallFromLoc(FVector Location)
{
	return GetWallAtTileSide(GetTileFromLoc(Location), ETileSide::Bottom);
}
	
AWall* AFloorBuilder::GetWallAtTileSide(FTileCoord Coord, ETileSide Dir)
{
	// UE_LOG(LogTemp, Warning, TEXT("Requested wall on %s"), *Coord.ToString());
	FString ID = GetWallID(Coord, Dir);
	AWall** Wall = AllWalls.Find(ID);
	if (Wall) return *Wall;
	return nullptr;
}

// Try to hide walls on the Tiles at this Location
// Called from the Character Every tick with the Actor location
void AFloorBuilder::OnUpdateCharCoord(FVector Location, ETileSide Dir)
{
	FTileCoord Coord = GetTileFromLoc(Location);
	if (Coord == LastEnteredTile) return;
	LastEnteredTile = Coord;
	UE_LOG(LogTemp, Warning, TEXT("Char at location %s, which is coord %s"), *Location.ToString(), *Coord.ToString());
	HideWall(Coord, Dir);
	FTileState* Room = GetTileAt(Coord);
	// this might actually be null if I dash into a pit UNDER the wall like an idiot
	if (!Room) return;
	// TODO pass location as parameter
	OnEnterTile(*Room);
	ATileMaster* Master = Room->TileMasterRef;
	// TODO this fails on coord 0,0 because game starts before room master spawns
	if (!Master) return;
	if (!Room->bIsCleared && Room->Template && Room->Template->bIsDoored && !Master->AreAllCharsDead())
	{
		//UE_LOG(LogTemp, Warning, TEXT("closing doors"));
		TeleportPlayerInsideTile(Location);
		//DebugTiles();
		if (Master) Master->bIsDoorOpen = false;
		CloseDoors();
	}
}

// Try to Hide walls on this Tiles Tile
void AFloorBuilder::HideWall(FTileCoord Coord, ETileSide Dir)
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
	AWall* ThisWall = GetWallAtTileSide(Coord, Dir);
	// Hide wall in given coord
	HideOneWall(ThisWall);
	FTileCoord RightCoord = GetNeighbor(Coord, ETileSide::Right);
	AWall* RightWall = GetWallAtTileSide(RightCoord, Dir);
	UTileTemplate* RightRoom = GetTemplateAt(RightCoord);
	// if room to the right is not doored, then hide it too
	if (RightRoom && !RightRoom->bIsDoored)	HideOneWall(RightWall);
	FTileCoord LeftCoord = GetNeighbor(Coord, ETileSide::Left);
	AWall* LeftWall = GetWallAtTileSide(LeftCoord, Dir);
	UTileTemplate* LeftRoom = GetTemplateAt(LeftCoord);
	// if room to the left is not doored, then hide it too
	if (LeftRoom && !LeftRoom->bIsDoored) HideOneWall(LeftWall);

}

void AFloorBuilder::HideOneWall(AWall* ThisWall)
{
	if (ThisWall)
	{
		ThisWall->Height = 50.f;
		ThisWall->BuildWalls();
		HiddenWalls.Add(ThisWall);
	}
}

void AFloorBuilder::SetTileCleared(FTileState& Room)
{
	Room.bIsCleared = true;
	OpenDoors();
}

void AFloorBuilder::OpenDoors()
{
	for (auto &&Door : DoorList)
	{
		Door->OpenDoor();
	}
}

/// <summary>
/// Closes all Doors from the DoorList Array
/// </summary>
void AFloorBuilder::CloseDoors()
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
void AFloorBuilder::TeleportPlayerInsideTile(FVector OldLocation)
{
	FTileCoord Coord = GetTileFromLoc(OldLocation);
	FVector NewLocation = GetLocFromTile(Coord);
	NewLocation.Z = OldLocation.Z;
	float Dist = 60.f;

	// try to find which door I came from
	float DistTop = NewLocation.X - OldLocation.X;
	float DistRight = NewLocation.Y - OldLocation.Y;
	if (FMath::Abs(DistRight) > FMath::Abs(DistTop))
	{
		// this means I came from either right or left
		if (NewLocation.Y > OldLocation.Y) NewLocation.Y -= (TileSizeY / 2 - Dist);	// from left
		else NewLocation.Y += (TileSizeY / 2 - Dist);								// from right
		NewLocation.X = OldLocation.X;
	}
	else
	{
		// this means I came from either top or bottom
		if (NewLocation.X > OldLocation.X) NewLocation.X -= (TileSizeX / 2 - Dist);	// from bottom
		else NewLocation.X += (TileSizeX / 2 - Dist);								// from top
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
void AFloorBuilder::AggroTile(APawn* PlayerToAggro, FVector Location)
{
	FTileState* Room = GetTileAt(GetTileFromLoc(Location));
	if (!Room || !Room->TileMasterRef) return;
	Room->TileMasterRef->AggroAll(PlayerToAggro);
}



