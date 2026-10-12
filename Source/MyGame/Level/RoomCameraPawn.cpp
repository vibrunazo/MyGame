#include "RoomCameraPawn.h"
#include "LevelBuilder.h"
#include "RunSubsystem.h"
#include "../Player/MyCharacter.h"

#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	// A character's height: what must stay in frame along the room's near (camera-side) edge
	constexpr float NearEdgeHeight = 200.f;
}

ARoomCameraPawn::ARoomCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(RootComponent);
	FollowCamera->bUsePawnControlRotation = false;
}

void ARoomCameraPawn::SetPlayerRef(AMyCharacter* NewPlayer)
{
	PlayerRef = NewPlayer;
}

FVector ARoomCameraPawn::GetCameraLoc()
{
	return FollowCamera ? FollowCamera->GetComponentLocation() : FVector::ZeroVector;
}

void ARoomCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!PlayerRef) return;
	if (!LevelBuilderRef) LevelBuilderRef = URunSubsystem::GetLevelBuilder(this);
	if (!LevelBuilderRef) return;

	SetActorRotation(FRotator(-Pitch, 0.f, 0.f));
	FollowCamera->SetFieldOfView(FieldOfView);
	if (!DarknessMID && RoomDarknessMaterial)
	{
		DarknessMID = UMaterialInstanceDynamic::Create(RoomDarknessMaterial, this);
		FollowCamera->PostProcessSettings.AddBlendable(DarknessMID, 1.f);
	}

	// a new window shape changes every framing
	const float Aspect = ViewportAspect();
	if (!FMath::IsNearlyEqual(Aspect, CachedAspect, 0.01f))
	{
		CachedAspect = Aspect;
		FramingCache.Reset();
		if (bHasRoom) ToLocation = FramingFor(CurrentRoom, Aspect);
	}

	const FIntPoint Room = RoomOfPlayer();
	if (!bHasRoom)
	{
		// first frame: no ease, just be there
		bHasRoom = true;
		CurrentRoom = Room;
		FromLocation = ToLocation = FramingFor(Room, Aspect);
		FromRect = ToRect = RectFor(Room);
		TransitionAlpha = 1.f;
	}
	else if (Room != CurrentRoom)
	{
		CurrentRoom = Room;
		FromLocation = GetActorLocation();
		FromRect = FMath::Lerp(FromRect, ToRect, FMath::SmoothStep(0.f, 1.f, TransitionAlpha));
		ToLocation = FramingFor(Room, Aspect);
		ToRect = RectFor(Room);
		TransitionAlpha = 0.f;
	}

	TransitionAlpha = FMath::Min(1.f, TransitionAlpha + DeltaTime / FMath::Max(TransitionTime, 0.01f));
	const float Eased = FMath::SmoothStep(0.f, 1.f, TransitionAlpha);
	SetActorLocation(FMath::Lerp(FromLocation, ToLocation, Eased));
	ApplyDarkness(FMath::Lerp(FromRect, ToRect, Eased));
}

FIntPoint ARoomCameraPawn::RoomOfPlayer()
{
	const FVector2D Size = RoomSize();
	const FVector P = PlayerRef->GetActorLocation();
	const FIntPoint Cell(FMath::RoundToInt(P.X / Size.X), FMath::RoundToInt(P.Y / Size.Y));
	if (!bHasRoom || Cell == CurrentRoom) return Cell;
	// switch only once the player is well inside the new room
	const float LocalX = FMath::Abs(P.X - Cell.X * Size.X), LocalY = FMath::Abs(P.Y - Cell.Y * Size.Y);
	const bool bInside = LocalX <= Size.X / 2 - SwitchMargin && LocalY <= Size.Y / 2 - SwitchMargin;
	return bInside ? Cell : CurrentRoom;
}

// Solves the camera location (fixed pitch and FoV) that frames the room's box as large as ScreenFill allows: the
// farthest the room's corners reach on screen equals ScreenFill, and the box is centered vertically
FVector ARoomCameraPawn::FramingFor(const FIntPoint& Cell, float Aspect)
{
	if (const FVector* Cached = FramingCache.Find(Cell)) return *Cached;
	const FVector2D Size = RoomSize();
	const FVector Center(Cell.X * Size.X, Cell.Y * Size.Y, 0.f);
	const float P = FMath::DegreesToRadians(Pitch);
	const FVector Forward(FMath::Cos(P), 0.f, -FMath::Sin(P)), Up(FMath::Sin(P), 0.f, FMath::Cos(P));
	const float TanH = FMath::Tan(FMath::DegreesToRadians(FieldOfView) / 2), TanV = TanH / FMath::Max(Aspect, 0.1f);
	TArray<FVector, TInlineAllocator<8>> Corners;
	for (float Y : { -Size.Y / 2, Size.Y / 2 })
	{
		Corners.Add(Center + FVector(-Size.X / 2, Y, 0.f));
		Corners.Add(Center + FVector(-Size.X / 2, Y, NearEdgeHeight));
		Corners.Add(Center + FVector(Size.X / 2, Y, 0.f));
		Corners.Add(Center + FVector(Size.X / 2, Y, FrameHeight));
	}
	// the screen extent of the corners for a camera Dist back from a pivot Shift ahead of the room's center
	auto Extent = [&](float Dist, float Shift, float& MinV, float& MaxV, float& MaxH)
	{
		const FVector Cam = Center + FVector(Shift, 0.f, 0.f) - Forward * Dist;
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
	const FVector Location = Center + FVector(Centered(Far), 0.f, 0.f) - Forward * Far;
	FramingCache.Add(Cell, Location);
	return Location;
}

FVector4 ARoomCameraPawn::RectFor(const FIntPoint& Cell) const
{
	const FVector2D Size = RoomSize();
	const FVector2D Center(Cell.X * Size.X, Cell.Y * Size.Y);
	const float HX = Size.X / 2 + DarknessMargin, HY = Size.Y / 2 + DarknessMargin;
	return FVector4(Center.X - HX, Center.Y - HY, Center.X + HX, Center.Y + HY);
}

FVector2D ARoomCameraPawn::RoomSize() const
{
	return LevelBuilderRef ? FVector2D(LevelBuilderRef->RoomSizeX, LevelBuilderRef->RoomSizeY) : FVector2D(2000.f, 2000.f);
}

float ARoomCameraPawn::ViewportAspect() const
{
	FVector2D Size(16.f, 9.f);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
	return Size.Y > 0.f ? Size.X / Size.Y : 16.f / 9.f;
}

void ARoomCameraPawn::ApplyDarkness(const FVector4& Rect)
{
	if (!DarknessMID) return;
	DarknessMID->SetScalarParameterValue(TEXT("RoomMinX"), Rect.X);
	DarknessMID->SetScalarParameterValue(TEXT("RoomMinY"), Rect.Y);
	DarknessMID->SetScalarParameterValue(TEXT("RoomMaxX"), Rect.Z);
	DarknessMID->SetScalarParameterValue(TEXT("RoomMaxY"), Rect.W);
	DarknessMID->SetScalarParameterValue(TEXT("Softness"), DarknessSoftness);
	DarknessMID->SetScalarParameterValue(TEXT("Brightness"), OutsideBrightness);
}
