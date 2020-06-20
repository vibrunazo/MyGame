// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomCameraPawn.h"
#include "../Player/MyCharacter.h"
#include "LevelBuilder.h"

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
	// CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	// CameraBoom->SetupAttachment(RootComponent);
	// CameraBoom->TargetArmLength = 700.0f; // The camera follows at this distance behind the character	
	// CameraBoom->SetWorldRotation(FRotator(-40.0f, 0.0f, 0.0f));
	// CameraBoom->bInheritYaw = false;
	// CameraBoom->bInheritPitch = false;
	// CameraBoom->bInheritRoll = false;
	// CameraBoom->bDoCollisionTest = false;
	// CameraBoom->bEnableCameraLag = true;
	// CameraBoom->CameraLagSpeed = 2.0f;
	// CameraBoom->CameraLagMaxDistance = 200.0f;
	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(RootComponent);
	// FollowCamera->SetRelativeLocation(FVector(0.f, 0.f, 500.f));
	// FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
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

	if (PlayerRef)
	{
		FollowPlayer(DeltaTime);
	}

}

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
	FVector RoomDistance = GetRoomDistance();
	// Target = PlayerRef->GetActorLocation()  + CameraDistance + RoomDistance/2.f;
	// Target.X -= TargetX * XRatio;
	float PositionInRoomX = TargetX - RoomDistance.X;
	float PositionInRoomY = PlayerRef->GetActorLocation().Y - RoomDistance.Y;
	FVector RoomSize = GetRoomSize();
	float CamMinX = -RoomSize.X/2 + RoomSize.X * MinXRatio + RoomDistance.X;
	float CamMaxX = -RoomSize.X/2 + RoomSize.X * MaxXRatio + RoomDistance.X;
	float AlphaX = PositionInRoomX/RoomSize.X + 0.5f;
	float CamMinY = -RoomSize.Y*0.5f*YRatio + RoomDistance.Y;
	float CamMaxY = +RoomSize.Y*0.5f*YRatio + RoomDistance.Y;
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
	if (TimeInCurrentDirection > 0.f)
	{
		ViewTarget.Y += RotOffset;
		RotLerpAlpha += RotSpeed * DeltaTime;
	}
	if (TimeInCurrentDirection < 0.f)
	{
		ViewTarget.Y -= RotOffset;
		RotLerpAlpha += RotSpeed * DeltaTime;
	}
	// lock X to player X
	RotLerpAlpha = FMath::Min(RotLerpAlpha, 1.f);
	ViewTarget.X = PlayerRef->GetActorLocation().X;
	ViewLoc = FMath::InterpEaseInOut(RotSource, ViewTarget, RotLerpAlpha, RotInterpExp);
	/*ViewLoc.X = FMath::FInterpTo(ViewLoc.X, ViewTarget.X, DeltaTime, RotSpeed);
	ViewLoc.Y = FMath::FInterpTo(ViewLoc.Y, ViewTarget.Y, DeltaTime, RotSpeed);
	ViewLoc.Z = FMath::FInterpTo(ViewLoc.Z, ViewTarget.Z, DeltaTime, RotSpeed);*/
	FRotator NewRot = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ViewLoc);
	SetActorRotation(NewRot);
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