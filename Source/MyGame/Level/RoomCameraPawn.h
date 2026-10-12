// The game camera: one fixed framing per room, like The Binding of Isaac. Inside a room it doesn't move; entering
// another room eases it over to that room's framing. The framing is solved at runtime so the whole room (walls,
// platforms, a character's height) fits the screen at the current aspect ratio, at a fixed pitch and FoV.
// Rooms other than the current one are darkened by a post-process material the camera keeps on its current room.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomCameraPawn.generated.h"

UCLASS()
class MYGAME_API ARoomCameraPawn : public AActor
{
	GENERATED_BODY()

public:
	ARoomCameraPawn();
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
	// The height framed above the room's floor: the tallest platforms plus a character standing on them
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	float FrameHeight = 600.f;
	// How much of the screen the room may fill (1 = edge to edge)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.5", ClampMax = "1"))
	float ScreenFill = 0.96f;
	// Seconds to ease over to the next room's framing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	float TransitionTime = 0.5f;
	// How far into another room the player must be before the camera switches to it (no flip-flopping on a border)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	float SwitchMargin = 150.f;

	// Darkens everything outside the current room (set on BP_RoomCameraPawn); none when unset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Darkness")
	TObjectPtr<class UMaterialInterface> RoomDarknessMaterial;
	// Brightness left outside the current room (0 black, 1 unchanged)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Darkness")
	float OutsideBrightness = 0.08f;
	// Width of the fade from the room's edge into the dark
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Darkness")
	float DarknessSoftness = 200.f;
	// How far past the room's edges stays lit (its own walls and doors)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Darkness")
	float DarknessMargin = 60.f;

private:
	// The grid cell the player stands in, with SwitchMargin hysteresis
	FIntPoint RoomOfPlayer();
	// Camera location framing a cell, cached per aspect ratio; the cell's rectangle (min x, min y, max x, max y)
	FVector FramingFor(const FIntPoint& Cell, float Aspect);
	FVector4 RectFor(const FIntPoint& Cell) const;
	FVector2D RoomSize() const;
	float ViewportAspect() const;
	void ApplyDarkness(const FVector4& Rect);

	UPROPERTY()
	TObjectPtr<class ALevelBuilder> LevelBuilderRef = nullptr;
	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> DarknessMID = nullptr;

	bool bHasRoom = false;
	FIntPoint CurrentRoom = FIntPoint::ZeroValue;
	// The ease between two framings: where it started, where it goes, how far along (0..1)
	FVector FromLocation = FVector::ZeroVector, ToLocation = FVector::ZeroVector;
	FVector4 FromRect, ToRect;
	float TransitionAlpha = 1.f;
	float CachedAspect = 0.f;
	TMap<FIntPoint, FVector> FramingCache;
};
