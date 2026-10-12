// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomCameraPawn.h"
//#include "LevelBuilder.h"
#include "../MyGameInstance.h"
#include "RunSubsystem.h"
#include "../Player/MyCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"

// A/B switch while you pick the new camera by feel (Phase 6); goes away with the old follow once picked
static TAutoConsoleVariable<int32> CVarLegacyCamera(TEXT("MyGame.LegacyCamera"), 0,
	TEXT("1: the 2020 camera follow (look-ahead offset, spring arm lag). 0: the framed follow (dead zone, no jump bob)."));

namespace
{
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
}

// Sets default values
ARoomCameraPawn::ARoomCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->ProjectileGravityScale = 0.f;
	MovementComponent->bIsHomingProjectile = true;
	MovementComponent->HomingAccelerationMagnitude = 10.f;
	MovementComponent->HomingTargetComponent;
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.0f; // The camera follows at this distance behind the character	
	//CameraBoom->SetWorldRotation(FRotator(-40.0f, 0.0f, 0.0f));
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 2.0f;
	CameraBoom->CameraLagMaxDistance = 200.0f;
	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	//FollowCamera->SetupAttachment(RootComponent);
	// FollowCamera->SetRelativeLocation(FVector(0.f, 0.f, 500.f));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

}

// Called when the game starts or when spawned
void ARoomCameraPawn::BeginPlay()
{
	Super::BeginPlay();

}

void ARoomCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PlayerRef && GetLevelBuilder())
	{
		TryRegisterEnterRoomEvent();
		const bool bLegacy = CVarLegacyCamera.GetValueOnGameThread() != 0;
		CameraBoom->bEnableCameraLag = bLegacy;  // the framed follow smooths itself
		if (bLegacy) FollowPlayer2(DeltaTime);
		else FollowPlayerFramed(DeltaTime);
		bFramedStarted = !bLegacy;
	}

}

/// <summary>
/// Currently called by the Player BP Begin Play
/// </summary>
/// <param name="NewPlayer"></param>
void ARoomCameraPawn::SetPlayerRef(AMyCharacter* NewPlayer)
{
	PlayerRef = NewPlayer;
	ViewLoc = PlayerRef->GetActorLocation();

	// MovementComponent->HomingTargetComponent = PlayerRef->GetRootComponent();
}

FVector ARoomCameraPawn::GetCameraLoc()
{
	if (FollowCamera) return FollowCamera->GetComponentLocation();
	return FVector();
}

/* Sets the Camera position and rotation depending on where the player is. */
void ARoomCameraPawn::FollowPlayer(float DeltaTime)
{
	// FollowCamera->RelativeRotation(NewRot);
	float TargetX = PlayerRef->GetActorLocation().X ;
	// Target = PlayerRef->GetActorLocation()  + FVector(-800.f - TargetX * 0.8, 0.f, 300.f);
	FVector RoomPosition = GetRoomDistance();
	// Target = PlayerRef->GetActorLocation()  + CameraDistance + RoomDistance/2.f;
	// Target.X -= TargetX * XRatio;
	float PositionInRoomX = TargetX - RoomPosition.X;
	float PositionInRoomY = PlayerRef->GetActorLocation().Y - RoomPosition.Y;
	FVector RoomSize = GetRoomSize();
	float CamMinX = -RoomSize.X/2 + RoomSize.X * MinXRatio + RoomPosition.X;
	float CamMaxX = -RoomSize.X/2 + RoomSize.X * MaxXRatio + RoomPosition.X;
	float AlphaX = PositionInRoomX/RoomSize.X + 0.5f;
	float CamMinY = -RoomSize.Y*0.5f*YRatio + RoomPosition.Y;
	float CamMaxY = +RoomSize.Y*0.5f*YRatio + RoomPosition.Y;
	float AlphaY = PositionInRoomY/RoomSize.Y + 0.5f;
	Target = FVector(
		FMath::Lerp(CamMinX, CamMaxX, AlphaX),
		FMath::Lerp(CamMinY, CamMaxY, AlphaY),
		PlayerRef->GetActorLocation().Z
		);
	float TargetFoV = FMath::Lerp(MinFoV, MaxFoV, AlphaX);
	float NewFoV = FMath::Lerp(FollowCamera->FieldOfView, TargetFoV, FoVLerp);
	FollowCamera->FieldOfView = NewFoV;

	FVector CurLoc = FMath::Lerp(GetActorLocation(), Target + CameraDistance, LerpSpeed);
	
	SetActorLocation(CurLoc);

	// FVector ViewTarget = PlayerRef->GetActorLocation() + PlayerRef->GetActorForwardVector() * ViewRotDistanceAhead;
	FVector ViewTarget = PlayerRef->GetActorLocation();
	// check if I'm looking left or right in this frame
	CurViewDistance = 0.f;
	if (PlayerRef->GetActorForwardVector().Y > 0.8) CurViewDistance = RotOffset;
	if (PlayerRef->GetActorForwardVector().Y < -0.8) CurViewDistance = -RotOffset;
	// What was the old direction camera was going last frame before updating
	float OldDir = TimeInCurrentDirection;
	TimeInCurrentDirection += DeltaTime * FMath::Sign(CurViewDistance);
	TimeInCurrentDirection = FMath::Clamp(TimeInCurrentDirection, -DelayBeforeStartsMoving, DelayBeforeStartsMoving);
	// check if I just changed direction on this frame
	if (OldDir * TimeInCurrentDirection < 0)
	{
		// if so, then set the Source of the Lerp to the current position
		RotSource = ViewLoc;
		RotLerpAlpha = 0.f;
	}
	//UE_LOG(LogTemp, Warning, TEXT("Sign: %f, TimeInDir: %f"), FMath::Sign(CurViewDistance), TimeInCurrentDirection);
	// set offset
	// If player moving right
	if (TimeInCurrentDirection > 0.f)
	{
		ViewTarget.Y += RotOffset;
		RotLerpAlpha += RotSpeed * DeltaTime;
	}
	// If player moving left
	if (TimeInCurrentDirection < 0.f)
	{
		ViewTarget.Y -= RotOffset;
		RotLerpAlpha += RotSpeed * DeltaTime;
	}
	// if current room has a wall to the right, limit how far the camera offset can go so it can't point past the wall
	if (bIsWalledRight) ViewTarget.Y = FMath::Min(ViewTarget.Y, RoomPosition.Y + RoomSize.Y*0.5f*RotWallClamp);
	// if current room has a wall to the left, limit how far the camera offset can go so it can't point past the wall
	if (bIsWalledLeft) ViewTarget.Y = FMath::Max(ViewTarget.Y, RoomPosition.Y - RoomSize.Y* 0.5f * RotWallClamp);
	// lock X to player X
	RotLerpAlpha = FMath::Min(RotLerpAlpha, 1.f);
	ViewTarget.X = PlayerRef->GetActorLocation().X;
	ViewLoc = FMath::InterpEaseInOut(RotSource, ViewTarget, RotLerpAlpha, RotInterpExp);
	/*ViewLoc.X = FMath::FInterpTo(ViewLoc.X, ViewTarget.X, DeltaTime, RotSpeed);
	ViewLoc.Y = FMath::FInterpTo(ViewLoc.Y, ViewTarget.Y, DeltaTime, RotSpeed);
	ViewLoc.Z = FMath::FInterpTo(ViewLoc.Z, ViewTarget.Z, DeltaTime, RotSpeed);*/
	FRotator NewRot = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ViewLoc);
	SetActorRotation(NewRot);
	//UE_LOG(LogTemp, Warning, TEXT("Cam rot target: %s, Left: %d, Right: %d"), *ViewTarget.ToString(), bIsWalledLeft, bIsWalledRight);
	// SetActorLocation(FVector(GetActorLocation().X, Target.Y, GetActorLocation().Z));

}

void ARoomCameraPawn::FollowPlayer2(float DeltaTime)
{
	FVector TargetLoc = PlayerRef->GetActorLocation();
	//OffsetTimeVector += PlayerRef->GetActorForwardVector().GetSafeNormal() * DeltaTime;
	OffsetTimeVector += (PlayerRef->GetActorForwardVector().GetSafeNormal()* DelayBeforeStartsMoving - OffsetTimeVector).GetSafeNormal()* DeltaTime;
	OffsetTimeVector = OffsetTimeVector.GetClampedToMaxSize(DelayBeforeStartsMoving);
	//if (OffsetTimeVector.Size() >= DelayBeforeStartsMoving) CurrentOffset = PlayerRef->GetActorForwardVector().GetSafeNormal() * RotOffset;
	if (OffsetTimeVector.Size() >= DelayBeforeStartsMoving * 0.95f)
	{
		CurrentOffset += (OffsetTimeVector.GetSafeNormal()* RotOffset - CurrentOffset).GetSafeNormal() * DeltaTime * OffsetAccel;
		CurrentOffset = CurrentOffset.GetClampedToMaxSize(RotOffset);
		//UE_LOG(LogTemp, Warning, TEXT("CurrentOffset: %s"), *CurrentOffset.ToString());
		//CurrentOffset = OffsetTimeVector.GetSafeNormal() * RotOffset;
	}
	//if (OffsetTimeVector.Size() >= DelayBeforeStartsMoving) CurrentOffset = FVector(0.f, 1.f, 0.f) * RotOffset;
	TargetLoc += CurrentOffset;

	/*FVector RoomPosition = CurrentRoomPosition;
	FVector RoomSize = CurrentRoomSize;*/
	FVector RoomPosition = GetRoomDistance();
	FVector RoomSize = CurrentRoomSize; 
	/*TargetLoc.Y = FMath::Clamp(TargetLoc.Y, RoomPosition.Y - 500.f, RoomPosition.Y + 500.f);
	TargetLoc.X = FMath::Clamp(TargetLoc.X, RoomPosition.X - 500.f, RoomPosition.X + 500.f);*/
	if (bIsWalledRight) TargetLoc.Y = FMath::Min(TargetLoc.Y, RoomPosition.Y + WallClampRight);
	if (bIsWalledLeft) TargetLoc.Y = FMath::Max(TargetLoc.Y, RoomPosition.Y - WallClampLeft);
	if (bIsWalledTop) TargetLoc.X = FMath::Min(TargetLoc.X, RoomPosition.X + WallClampTop);
	if (bIsWalledBottom) TargetLoc.X = FMath::Max(TargetLoc.X, RoomPosition.X - WallClampBottom);

	SetActorLocation(TargetLoc);
	//UE_LOG(LogTemp, Warning, TEXT("OffsetTime: %s, Size: %f"), *OffsetTimeVector.ToString(), OffsetTimeVector.Size());
	//UE_LOG(LogTemp, Warning, TEXT("RotOffset: %f"), RotOffset);
	//DrawDebugLine(GetWorld(), PlayerRef->GetActorLocation(), PlayerRef->GetActorLocation()+OffsetTimeVector*100.f, FColor::Red);
	//DrawDebugDirectionalArrow(GetWorld(), PlayerRef->GetActorLocation(), PlayerRef->GetActorLocation() + OffsetTimeVector * 100.f, 5.f, FColor::Red);
}

void ARoomCameraPawn::FollowPlayerFramed(float DeltaTime)
{
	const FVector Player = PlayerRef->GetActorLocation();
	if (!bFramedStarted)
	{
		Focus = Player;
		SmoothVelocity = LookAheadVelocity = LookAheadTarget = LookAheadOffset = MoveDirection = FVector::ZeroVector;
		MoveTime = 0.f;
	}
	// the dead zone: drag the focus only by how far the player is past the box's edge
	const float DX = Player.X - Focus.X, DY = Player.Y - Focus.Y;
	if (FMath::Abs(DX) > DeadZone.X) Focus.X = Player.X - FMath::Sign(DX) * DeadZone.X;
	if (FMath::Abs(DY) > DeadZone.Y) Focus.Y = Player.Y - FMath::Sign(DY) * DeadZone.Y;
	// height follows floors and stairs, not jumps, launches or knock-ups
	const UCharacterMovementComponent* Movement = PlayerRef->GetCharacterMovement();
	if (!bFramedStarted || (Movement && Movement->IsMovingOnGround())) Focus.Z = Player.Z;

	// look ahead where the player has been heading for a moment; standing still (fighting, shooting) keeps it
	const FVector Velocity2D = FVector(PlayerRef->GetVelocity().X, PlayerRef->GetVelocity().Y, 0.f);
	if (Velocity2D.Size() > 100.f)
	{
		const FVector Direction = Velocity2D.GetSafeNormal();
		MoveTime = (FVector::DotProduct(Direction, MoveDirection) > 0.7f) ? MoveTime + DeltaTime : 0.f;
		if (MoveTime == 0.f) MoveDirection = Direction;
		if (MoveTime >= LookAheadDelay) LookAheadTarget = MoveDirection * LookAhead;
	}
	LookAheadOffset = SmoothDamp(LookAheadOffset, LookAheadTarget, LookAheadVelocity, LookAheadSmoothTime, DeltaTime);

	// don't look past a room's walls (the same clamps as the old follow)
	const FVector RoomPosition = GetRoomDistance();
	FVector Goal = Focus + LookAheadOffset;
	if (bIsWalledRight) Goal.Y = FMath::Min(Goal.Y, RoomPosition.Y + WallClampRight);
	if (bIsWalledLeft) Goal.Y = FMath::Max(Goal.Y, RoomPosition.Y - WallClampLeft);
	if (bIsWalledTop) Goal.X = FMath::Min(Goal.X, RoomPosition.X + WallClampTop);
	if (bIsWalledBottom) Goal.X = FMath::Max(Goal.X, RoomPosition.X - WallClampBottom);

	SetActorLocation(bFramedStarted ? SmoothDamp(GetActorLocation(), Goal, SmoothVelocity, SmoothTime, DeltaTime) : Goal);
}

// The level builder's room size; 2000 x 2000 without one
FVector ARoomCameraPawn::GetRoomSize()
{
	if (ALevelBuilder* Builder = GetLevelBuilder()) return FVector(Builder->RoomSizeX, Builder->RoomSizeY, 0.f);
	return FVector(2000.f, 2000.f, 0.f);
}

ALevelBuilder* ARoomCameraPawn::GetLevelBuilder()
{
	if (!LevelBuilderRef) LevelBuilderRef = URunSubsystem::GetLevelBuilder(this);
	return LevelBuilderRef;
}

void ARoomCameraPawn::TryRegisterEnterRoomEvent()
{
	if (bIsRoomEnterRegistered) return;
	if (!GetLevelBuilder()) return;
	LevelBuilderRef->OnEnterRoomDelegate.AddDynamic(this, &ARoomCameraPawn::OnEnterRoom);
	

	bIsRoomEnterRegistered = true;
}

void ARoomCameraPawn::OnEnterRoom(FRoomState NewRoom)
{
	UE_LOG(LogTemp, Warning, TEXT("Running Enter Room event on RoomCamera, Room: %s, left: %d, right: %d"), NewRoom.RoomType ? *NewRoom.RoomType->Level.GetAssetName() : TEXT("none"), NewRoom.Walls.Contains(ERoomDirection::Left), NewRoom.Walls.Contains(ERoomDirection::Right));
	// if I just came from a room that had a wall in the direction that I'm facing, then reset the rot interp
	if ((TimeInCurrentDirection > 0.f && bIsWalledRight) || (TimeInCurrentDirection < 0.f && bIsWalledLeft))
	{
		RotSource = ViewLoc;
		RotLerpAlpha = 0.1f;
	}
	bIsWalledLeft = NewRoom.Walls.Contains(ERoomDirection::Left);
	bIsWalledRight = NewRoom.Walls.Contains(ERoomDirection::Right);
	bIsWalledTop = NewRoom.Walls.Contains(ERoomDirection::Top);
	bIsWalledBottom = NewRoom.Walls.Contains(ERoomDirection::Bottom);
	CurrentRoomRef = &NewRoom;
	// TODO get and record current room position from passed parameter from level builder

	/*if (!GetLevelBuilder()) return;
	CurrentRoomSize = FVector(GetLevelBuilder()->RoomSizeX, GetLevelBuilder()->RoomSizeX, 0.f);
	float RoomSizeX = CurrentRoomSize.X;
	float RoomSizeY = CurrentRoomSize.Y;
	int16 NewX = FMath::RoundToInt(PlayerRef->GetActorLocation().X / RoomSizeX);
	int16 NewY = FMath::RoundToInt(PlayerRef->GetActorLocation().Y / RoomSizeY);
	CurrentRoomPosition = FVector(NewX * RoomSizeX, NewY * RoomSizeY, 0.f);
	UE_LOG(LogTemp, Warning, TEXT("updated room position to %s: "), *CurrentRoomPosition.ToString());*/

}

/* Returns the location of the room the player is currently at. Used to position the camera next to the current room
after the player changes rooms. */
FVector ARoomCameraPawn::GetRoomDistance()
{
	FVector RoomDistance = FVector(0.f, 0.f, 0.f);
	float RoomSizeX = 2000.f;
	float RoomSizeY = 2000.f;
	RoomSizeX = GetRoomSize().X;
	RoomSizeY = GetRoomSize().Y;
	int16 NewX = FMath::RoundToInt(PlayerRef->GetActorLocation().X / RoomSizeX);
	int16 NewY = FMath::RoundToInt(PlayerRef->GetActorLocation().Y / RoomSizeY);
	RoomDistance = FVector(NewX * RoomSizeX, NewY * RoomSizeY, 0.f);
	return RoomDistance;
}