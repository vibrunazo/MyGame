#include "RoomCamera.h"
#include "FloorBuilder.h"
#include "RunSubsystem.h"
#include "../Player/MyCharacter.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	// A character's height: what must stay in frame along a tile's near (camera-side) edge
	constexpr float NearEdgeHeight = 200.f;

	// Critically damped smoothing toward Target (Game Programming Gems 4, 1.10): no overshoot, continuous speed
	FVector SmoothDamp(const FVector& Current, const FVector& Target, FVector& Velocity, float SmoothTime, float DeltaTime)
	{
		const float Omega = 2.f / FMath::Max(SmoothTime, 1e-4f);
		const float X = Omega * DeltaTime;
		const float Exp = 1.f / (1.f + X + 0.48f * X * X + 0.235f * X * X * X);
		const FVector Change = Current - Target;
		const FVector Temp = (Velocity + Omega * Change) * DeltaTime;
		Velocity = (Velocity - Omega * Temp) * Exp;
		return Target + (Change + Temp) * Exp;
	}

	FVector2D ClosestOnSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const float T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1.f), 0.f, 1.f);
		return A + AB * T;
	}
}

ARoomCamera::ARoomCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(RootComponent);
	FollowCamera->bUsePawnControlRotation = false;
}

void ARoomCamera::SetPlayerRef(AMyCharacter* NewPlayer)
{
	PlayerRef = NewPlayer;
}

FVector ARoomCamera::GetCameraLoc()
{
	return FollowCamera ? FollowCamera->GetComponentLocation() : FVector::ZeroVector;
}

void ARoomCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!PlayerRef) return;
	if (!FloorBuilderRef) FloorBuilderRef = URunSubsystem::GetFloorBuilder(this);
	if (!FloorBuilderRef || FloorBuilderRef->NumRooms() == 0) return;

	SetActorRotation(FRotator(-Pitch, 0.f, 0.f));
	FollowCamera->SetFieldOfView(FieldOfView);
	if (!DarknessMID && RoomDarknessMaterial)
	{
		DarknessMID = UMaterialInstanceDynamic::Create(RoomDarknessMaterial, this);
		FollowCamera->PostProcessSettings.AddBlendable(DarknessMID, 1.f);
		BuildMask();
	}
	const float Aspect = ViewportAspect();
	if (!FMath::IsNearlyEqual(Aspect, CachedAspect, 0.01f))
	{
		CachedAspect = Aspect;
		FramingOffset = SolveFramingOffset(Aspect);
	}

	const FVector Player = PlayerRef->GetActorLocation();
	CurrentTile = TileOfPlayer();
	const int32 Room = FloorBuilderRef->GetRoomOfTile(CurrentTile);
	if (!bStarted)
	{
		// first frame: just be there
		bStarted = true;
		CurrentRoom = PreviousRoom = Room;
		Focus = FocusIn(Room, Player);
		FocusVelocity = FVector::ZeroVector;
		TransitionAlpha = 1.f;
		SetActorLocation(Focus + FramingOffset);
		PaintMask(Room, Room, 1.f);
		return;
	}
	if (Room >= 0 && Room != CurrentRoom)
	{
		// through a door: ease from here to the new room's framing
		PreviousRoom = CurrentRoom;
		CurrentRoom = Room;
		TransitionFrom = GetActorLocation();
		TransitionAlpha = 0.f;
		Focus = FocusIn(Room, Player);
		FocusVelocity = FVector::ZeroVector;
	}

	Focus = SmoothDamp(Focus, FocusIn(CurrentRoom, Player), FocusVelocity, FollowSmoothTime, DeltaTime);
	FVector Location = Focus + FramingOffset;
	if (TransitionAlpha < 1.f)
	{
		TransitionAlpha = FMath::Min(1.f, TransitionAlpha + DeltaTime / FMath::Max(TransitionTime, 0.01f));
		const float Eased = FMath::SmoothStep(0.f, 1.f, TransitionAlpha);
		Location = FMath::Lerp(TransitionFrom, Location, Eased);
		PaintMask(PreviousRoom, CurrentRoom, Eased);
	}
	SetActorLocation(Location);
}

FIntPoint ARoomCamera::TileOfPlayer()
{
	const FVector2D Size = TileSize();
	const FVector P = PlayerRef->GetActorLocation();
	const FIntPoint Tile(FMath::RoundToInt(P.X / Size.X), FMath::RoundToInt(P.Y / Size.Y));
	if (!bStarted || Tile == CurrentTile || FloorBuilderRef->GetRoomOfTile(Tile) < 0) return bStarted ? CurrentTile : Tile;
	// switch only once the player is well inside the new tile
	const float LocalX = FMath::Abs(P.X - Tile.X * Size.X), LocalY = FMath::Abs(P.Y - Tile.Y * Size.Y);
	return LocalX <= Size.X / 2 - SwitchMargin && LocalY <= Size.Y / 2 - SwitchMargin ? Tile : CurrentTile;
}

// The room's spine: its tile centers, the lines between joined neighbors, and the squares of 2 x 2 blocks. The focus
// is the spine's point nearest the player, so a one-tile room has a single focus and the view never leaves the room
FVector ARoomCamera::FocusIn(int32 Room, const FVector& Player) const
{
	const TArray<FIntPoint>& Tiles = FloorBuilderRef->GetRoomTiles(Room);
	if (Tiles.IsEmpty()) return FVector(Player.X, Player.Y, 0.f);
	const FVector2D Size = TileSize();
	const FVector2D P(Player.X, Player.Y);
	auto Center = [&](const FIntPoint& T) { return FVector2D(T.X * Size.X, T.Y * Size.Y); };
	FVector2D Best = Center(Tiles[0]);
	float BestDist = FVector2D::DistSquared(P, Best);
	auto Consider = [&](const FVector2D& C)
	{
		const float D = FVector2D::DistSquared(P, C);
		if (D < BestDist) { BestDist = D; Best = C; }
	};
	for (const FIntPoint& T : Tiles)
	{
		Consider(Center(T));
		for (const FIntPoint& Step : { FIntPoint(1, 0), FIntPoint(0, 1) })
		{
			if (FloorBuilderRef->AreJoined(T, T + Step)) Consider(ClosestOnSegment(P, Center(T), Center(T + Step)));
		}
		// a 2 x 2 block of joined tiles: anywhere in between
		const FIntPoint A = T + FIntPoint(1, 0), B = T + FIntPoint(0, 1), C = T + FIntPoint(1, 1);
		if (FloorBuilderRef->AreJoined(T, A) && FloorBuilderRef->AreJoined(T, B) && FloorBuilderRef->AreJoined(A, C) && FloorBuilderRef->AreJoined(B, C))
		{
			Consider(FVector2D(FMath::Clamp(P.X, Center(T).X, Center(C).X), FMath::Clamp(P.Y, Center(T).Y, Center(C).Y)));
		}
	}
	return FVector(Best.X, Best.Y, 0.f);
}

// The camera's offset from the focus point at a fixed pitch and FoV: as close as possible while a tile's box (its
// floor, its far side up to FrameHeight, a character along the near side) stays within ScreenFill, centered vertically
FVector ARoomCamera::SolveFramingOffset(float Aspect) const
{
	const FVector2D Size = TileSize();
	const float P = FMath::DegreesToRadians(Pitch);
	const FVector Forward(FMath::Cos(P), 0.f, -FMath::Sin(P)), Up(FMath::Sin(P), 0.f, FMath::Cos(P));
	const float TanH = FMath::Tan(FMath::DegreesToRadians(FieldOfView) / 2), TanV = TanH / FMath::Max(Aspect, 0.1f);
	TArray<FVector, TInlineAllocator<8>> Corners;
	for (float Y : { -Size.Y / 2, Size.Y / 2 })
	{
		Corners.Add(FVector(-Size.X / 2, Y, 0.f));
		Corners.Add(FVector(-Size.X / 2, Y, NearEdgeHeight));
		Corners.Add(FVector(Size.X / 2, Y, 0.f));
		Corners.Add(FVector(Size.X / 2, Y, FrameHeight));
	}
	auto Extent = [&](float Dist, float Shift, float& MinV, float& MaxV, float& MaxH)
	{
		const FVector Cam = FVector(Shift, 0.f, 0.f) - Forward * Dist;
		MinV = MAX_flt; MaxV = -MAX_flt; MaxH = 0.f;
		for (const FVector& C : Corners)
		{
			const FVector V = C - Cam;
			const float Depth = FMath::Max(FVector::DotProduct(V, Forward), 1.f);
			const float SV = FVector::DotProduct(V, Up) / (Depth * TanV);
			MinV = FMath::Min(MinV, SV); MaxV = FMath::Max(MaxV, SV);
			MaxH = FMath::Max(MaxH, FMath::Abs(V.Y) / (Depth * TanH));
		}
	};
	// for a distance, the shift that centers the box vertically (moving the pivot ahead moves the box down)
	auto Centered = [&](float Dist)
	{
		float Lo = -Size.X, Hi = Size.X;
		for (int32 i = 0; i < 40; i++)
		{
			float MinV, MaxV, MaxH;
			const float Mid = (Lo + Hi) / 2;
			Extent(Dist, Mid, MinV, MaxV, MaxH);
			(MinV + MaxV > 0.f ? Lo : Hi) = Mid;
		}
		return (Lo + Hi) / 2;
	};
	float Near = 100.f, Far = 50000.f;
	for (int32 i = 0; i < 40; i++)
	{
		const float Dist = (Near + Far) / 2;
		float MinV, MaxV, MaxH;
		Extent(Dist, Centered(Dist), MinV, MaxV, MaxH);
		(FMath::Max3(-MinV, MaxV, MaxH) > ScreenFill ? Near : Far) = Dist;
	}
	return FVector(Centered(Far), 0.f, 0.f) - Forward * Far;
}

FVector2D ARoomCamera::TileSize() const
{
	return FloorBuilderRef ? FVector2D(FloorBuilderRef->TileSizeX, FloorBuilderRef->TileSizeY) : FVector2D(2000.f, 2000.f);
}

float ARoomCamera::ViewportAspect() const
{
	FVector2D Size(16.f, 9.f);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
	return Size.Y > 0.f ? Size.X / Size.Y : 16.f / 9.f;
}

// One mask texel block per tile over the floor (plus a tile of margin): lit for the current room's tiles. Bilinear
// filtering between texels makes the soft edge at the room's walls
void ARoomCamera::BuildMask()
{
	FIntPoint Min, Max;
	FloorBuilderRef->GetTileExtent(Min, Max);
	MaskOrigin = Min - FIntPoint(1, 1);
	MaskTiles = Max - Min + FIntPoint(3, 3);
	const int32 W = MaskTiles.X * MaskTexelsPerTile, H = MaskTiles.Y * MaskTexelsPerTile;
	RoomMask = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
	RoomMask->SRGB = false;
	RoomMask->Filter = TF_Bilinear;
	RoomMask->AddressX = RoomMask->AddressY = TA_Clamp;
	RoomMask->UpdateResource();
	MaskPixels.SetNumZeroed(W * H * 4);

	const FVector2D Size = TileSize();
	DarknessMID->SetTextureParameterValue(TEXT("RoomMask"), RoomMask);
	DarknessMID->SetScalarParameterValue(TEXT("OriginX"), (MaskOrigin.X - 0.5f) * Size.X);
	DarknessMID->SetScalarParameterValue(TEXT("OriginY"), (MaskOrigin.Y - 0.5f) * Size.Y);
	DarknessMID->SetScalarParameterValue(TEXT("ExtentX"), MaskTiles.X * Size.X);
	DarknessMID->SetScalarParameterValue(TEXT("ExtentY"), MaskTiles.Y * Size.Y);
	DarknessMID->SetScalarParameterValue(TEXT("Brightness"), OutsideBrightness);
}

void ARoomCamera::PaintMask(int32 FromRoom, int32 ToRoom, float Alpha)
{
	if (!RoomMask) return;
	const int32 W = MaskTiles.X * MaskTexelsPerTile, H = MaskTiles.Y * MaskTexelsPerTile;
	FMemory::Memzero(MaskPixels.GetData(), MaskPixels.Num());
	for (int32 TX = 0; TX < MaskTiles.X; TX++)
	{
		for (int32 TY = 0; TY < MaskTiles.Y; TY++)
		{
			const int32 Room = FloorBuilderRef->GetRoomOfTile(MaskOrigin + FIntPoint(TX, TY));
			if (Room < 0) continue;
			const float Lit = FMath::Lerp(Room == FromRoom ? 1.f : 0.f, Room == ToRoom ? 1.f : 0.f, Alpha);
			const uint8 V = (uint8)FMath::RoundToInt(Lit * 255.f);
			if (!V) continue;
			// texture u runs along world X, v along world Y
			for (int32 U = TX * MaskTexelsPerTile; U < (TX + 1) * MaskTexelsPerTile; U++)
			{
				for (int32 Vv = TY * MaskTexelsPerTile; Vv < (TY + 1) * MaskTexelsPerTile; Vv++)
				{
					uint8* Px = &MaskPixels[(Vv * W + U) * 4];
					Px[0] = Px[1] = Px[2] = V;
					Px[3] = 255;
				}
			}
		}
	}
	uint8* Data = (uint8*)FMemory::Malloc(MaskPixels.Num());
	FMemory::Memcpy(Data, MaskPixels.GetData(), MaskPixels.Num());
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, W, H);
	RoomMask->UpdateTextureRegions(0, 1, Region, W * 4, 4, Data, [](uint8* SrcData, const FUpdateTextureRegion2D* Regions)
	{
		FMemory::Free(SrcData);
		delete Regions;
	});
}
