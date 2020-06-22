// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomCameraPawn.h"
#include "LevelBuilder.h"
#include "../MyGameInstance.h"
#include "../Player/MyCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

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
	CameraBoom->TargetArmLength = 50.0f; // The camera follows at this distance behind the character	
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

	if (PlayerRef && GetMyGameInstance() && GetLevelBuilder())
	{
		TryRegisterEnterRoomEvent();
		FollowPlayer(DeltaTime);
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

/* Returns a FVector with the room size. Will try to get this value from the LevelBuilder placed in the world.
If it finds the LevelBuilder will record a reference to it to use in future calls.
If it fails defaults to 2000x2000x0 Room Size. */
FVector ARoomCameraPawn::GetRoomSize()
{
	FVector RoomSize = FVector(2000.f, 2000.f, 0.f);
	if (!LevelBuilderRef)
	{
		TArray<AActor*> OutActors;
		// TODO DON'T DO THIS WTF
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALevelBuilder::StaticClass(), OutActors);
		if (OutActors.Num() > 0) LevelBuilderRef = Cast<ALevelBuilder>(OutActors[0]);
	}
	if (LevelBuilderRef) RoomSize = FVector(LevelBuilderRef->RoomSizeX, LevelBuilderRef->RoomSizeY, 0.f);
	return RoomSize;
}

UMyGameInstance* ARoomCameraPawn::GetMyGameInstance()
{
	if (MyGameInstanceRef) return MyGameInstanceRef;
	UMyGameInstance* result = Cast<UMyGameInstance>(GetGameInstance());
	if (result)
	{
		MyGameInstanceRef = result;
		return result;
	}
	return nullptr;
}

ALevelBuilder* ARoomCameraPawn::GetLevelBuilder()
{
	if (LevelBuilderRef) return LevelBuilderRef;
	if (!GetMyGameInstance()) return nullptr;
	ALevelBuilder* result = Cast<ALevelBuilder>(GetMyGameInstance()->GetLevelBuilder());
	if (result)
	{
		LevelBuilderRef = result;
		return result;
	}
	return nullptr;
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
	UE_LOG(LogTemp, Warning, TEXT("Running Enter Room event on RoomCamera, Room: %s, left: %d, right: %d"), *NewRoom.RoomType->LevelAddress.ToString(), NewRoom.Walls.Contains(EDirection::Left), NewRoom.Walls.Contains(EDirection::Right));
	// if I just came from a room that had a wall in the direction that I'm facing, then reset the rot interp
	if ((TimeInCurrentDirection > 0.f && bIsWalledRight) || (TimeInCurrentDirection < 0.f && bIsWalledLeft))
	{
		RotSource = ViewLoc;
		RotLerpAlpha = 0.1f;
	}
	bIsWalledLeft = NewRoom.Walls.Contains(EDirection::Left);
	bIsWalledRight = NewRoom.Walls.Contains(EDirection::Right);
	CurrentRoomRef = &NewRoom;
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
	int16 NewX = FMath::DivideAndRoundNearest(PlayerRef->GetActorLocation().X, RoomSizeX);
	int16 NewY = FMath::DivideAndRoundNearest(PlayerRef->GetActorLocation().Y, RoomSizeY);
	RoomDistance = FVector(NewX * RoomSizeX, NewY * RoomSizeY, 0.f);
	return RoomDistance;
}