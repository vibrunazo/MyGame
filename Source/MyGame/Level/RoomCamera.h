// The game camera, confined to the room the player is in (a room = tiles with no wall between them, see CLAUDE.md).
// It frames one tile at a fixed pitch and FoV, solved at runtime so a whole tile fits the screen at the current aspect
// ratio. In a one-tile room it doesn't move. In a bigger room it slides along the room's spine (its tile centers and
// the lines between neighboring tiles), so it never shows past the room's walls. Through a door into another room it
// eases over. Everything outside the current room is darkened by a post-process material fed a mask of the room's tiles.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomCamera.generated.h"

UCLASS()
class MYGAME_API ARoomCamera : public AActor
{
	GENERATED_BODY()

public:
	ARoomCamera();
	virtual void Tick(float DeltaTime) override;
	// Called by the player's Blueprint at BeginPlay
	UFUNCTION(BlueprintCallable, Category = RoomCamera)
	void SetPlayerRef(class AMyCharacter* NewPlayer);
	FVector GetCameraLoc();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	TObjectPtr<class UCameraComponent> FollowCamera;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	TObjectPtr<class AMyCharacter> PlayerRef;

	// How far down the camera looks (degrees)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	float Pitch = 40.f;
	// Horizontal field of view (degrees)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	float FieldOfView = 60.f;
	// The height framed above a tile's floor: the tallest platforms plus a character standing on them
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	float FrameHeight = 600.f;
	// How much of the screen a tile may fill (1 = edge to edge)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.5", ClampMax = "1"))
	float ScreenFill = 0.96f;
	// Seconds the camera takes to catch up while sliding inside a big room (critically damped, no overshoot)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	float FollowSmoothTime = 0.45f;
	// Seconds to ease over into another room
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	float TransitionTime = 0.5f;
	// How far into another room's tile the player must be before the camera switches (no flip-flopping in a doorway)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	float SwitchMargin = 150.f;

	// Darkens everything outside the current room (set on BP_RoomCamera); none when unset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Darkness")
	TObjectPtr<class UMaterialInterface> RoomDarknessMaterial;
	// Brightness left outside the current room (0 black, 1 unchanged)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Darkness")
	float OutsideBrightness = 0.08f;
	// Mask texels per tile side: more makes the fade at the room's edge narrower
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Darkness", meta = (ClampMin = "2", ClampMax = "32"))
	int32 MaskTexelsPerTile = 8;

private:
	// The tile the player stands in, switching only once well inside a new one
	FIntPoint TileOfPlayer();
	// Where the camera's focus goes in a room: the point of the room's spine nearest the player
	FVector FocusIn(int32 Room, const FVector& Player) const;
	// The camera's offset from the focus point, solved so a tile fills the screen (per aspect ratio)
	FVector SolveFramingOffset(float Aspect) const;
	FVector2D TileSize() const;
	float ViewportAspect() const;
	void BuildMask();
	void PaintMask(int32 FromRoom, int32 ToRoom, float Alpha);

	UPROPERTY()
	TObjectPtr<class AFloorBuilder> FloorBuilderRef = nullptr;
	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> DarknessMID = nullptr;
	UPROPERTY()
	TObjectPtr<class UTexture2D> RoomMask = nullptr;

	bool bStarted = false;
	FIntPoint CurrentTile = FIntPoint::ZeroValue;
	int32 CurrentRoom = -1, PreviousRoom = -1;
	FVector Focus = FVector::ZeroVector, FocusVelocity = FVector::ZeroVector;
	// The ease into another room: where the camera was, how far along (0..1)
	FVector TransitionFrom = FVector::ZeroVector;
	float TransitionAlpha = 1.f;
	float CachedAspect = 0.f;
	FVector FramingOffset = FVector::ZeroVector;
	// The mask covers the floor's tiles plus one tile of margin: its first tile and size in tiles
	FIntPoint MaskOrigin = FIntPoint::ZeroValue, MaskTiles = FIntPoint::ZeroValue;
	TArray<uint8> MaskPixels;
};
