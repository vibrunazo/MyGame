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

	// UE_LOG(LogTemp, Warning, TEXT("PositionInRoomX: %f, AlphaX: %f, PositionInRoomY: %f, AlphaY: %f, RoomDistance: %s"), PositionInRoomX, AlphaX, PositionInRoomY, AlphaY, *RoomDistance.ToString());
	FVector CurLoc = FMath::Lerp(GetActorLocation(), Target + CameraDistance, LerpSpeed);
	// CurLoc += FVector(-500.f, 0.f, 400.f);

	SetActorLocation(CurLoc);

	FVector ViewTarget = PlayerRef->GetActorLocation() + PlayerRef->GetActorForwardVector() * ViewRotDistanceAhead;
	// Clamp the View Target X axis to stay inside the room, if the player is looking towards bottom, then the camera won't point to the bottom room
	ViewTarget.X = FMath::Clamp(ViewTarget.X, -RoomSize.X*0.3f + RoomDistance.X, +RoomSize.X*0.5f + RoomDistance.X);
	// to avoid going past the target, check if the amount I would move isn't greater than the distance to target, if it is, snap to target
	FVector ViewDelta = (ViewTarget - ViewLoc).GetSafeNormal() * RotSpeed * DeltaTime;
	// ViewVelocity += ViewDelta;
	// FVector::DotProduct(ViewTarget - ViewLoc, ViewVelocity);
	// ViewVelocity = (ViewTarget - ViewLoc).GetSafeNormal() * ViewVelocity.Size() + ViewDelta;
	// ViewVelocity = (ViewTarget - ViewLoc).GetSafeNormal() * FVector::DotProduct(ViewVelocity, ViewTarget - ViewLoc) + ViewDelta;
	// if the angle between the current velocity and the target is too high (cosine between them too low), then stop moving
	if (FVector::DotProduct(ViewTarget - ViewLoc, ViewVelocity)/((ViewTarget - ViewLoc).Size()*ViewVelocity.Size()) < 0.8)
	{
		// UE_LOG(LogTemp, Warning, TEXT("stopping"));
		// ViewVelocity = FVector(0.f, 0.f, 0.f);
		ViewVelocity *= RotBreak;
	}
	// float cos = CurVector.CosineAngle2D(LastInputVector);
	// 		float acos = UKismetMathLibrary::DegAcos(cos);
	// TODO might not need dotproduct and still get same result because I'm already breaking on low cosine
	ViewVelocity = (ViewTarget - ViewLoc).GetSafeNormal() * ViewVelocity.Size() + ViewDelta;
	// ViewVelocity = ((ViewTarget - ViewLoc) * FVector::DotProduct(ViewTarget - ViewLoc, ViewVelocity)).GetSafeNormal() * ViewVelocity.Size() + ViewDelta ;
	// ViewVelocity = ViewVelocity * FVector::DotProduct(ViewVelocity, ViewTarget - ViewLoc);
	// if (bDrawDebugLines) DrawDebugDirectionalArrow(GetWorld(), ViewLoc, ViewLoc + ViewVelocity, 10.f, FColor::Turquoise, false, -1.f, 10, 2.f);
	// ViewVelocity += ViewDelta;
	// DrawDebugDirectionalArrow(GetWorld(), ViewLoc, ViewLoc + ViewVelocity, 10.f, FColor::Green, false, -1.f, 10, 2.f);
	// UE_LOG(LogTemp, Warning, TEXT("dot product: %f"), FVector::DotProduct(ViewVelocity, ViewTarget - ViewLoc));
	ViewVelocity = ViewVelocity.GetClampedToMaxSize(RotMaxSpeed);
	if (bDrawDebugLines) DrawDebugDirectionalArrow(GetWorld(), ViewLoc, ViewTarget, 10.f, FColor::Green, false, -1.f, 10, 4.f);
	if (bDrawDebugLines) DrawDebugDirectionalArrow(GetWorld(), ViewLoc, ViewLoc + ViewVelocity, 10.f, FColor::Red, false, -1.f, 10, 2.f);
	if (bDrawDebugLines) DrawDebugDirectionalArrow(GetWorld(), FVector::ZeroVector, (ViewTarget - ViewLoc).GetSafeNormal() * 100.f, 10.f, FColor::White, false, -1.f, 10, 2.f);
	if (bDrawDebugLines) DrawDebugDirectionalArrow(GetWorld(), FVector::ZeroVector, ViewVelocity.GetSafeNormal() * 50.f, 10.f, FColor::Red, false, -1.f, 10, 5.f);
	if (bDrawDebugLines) DrawDebugDirectionalArrow(GetWorld(), FVector::ZeroVector, ((ViewTarget - ViewLoc).GetSafeNormal() * FVector::DotProduct(ViewTarget - ViewLoc, ViewVelocity)).GetSafeNormal() * 20.f, 10.f, FColor::Blue, false, -1.f, 10, 2.f);
	// UE_LOG(LogTemp, Warning, TEXT("ViewLoc: %s, ViewTarget-Loc: %f, ViewVelocity: %f, Dot: %f"), *ViewLoc.ToString(), (ViewTarget - ViewLoc).Size(), ViewVelocity.Size(), FMath::Sign(FVector::DotProduct(ViewTarget - ViewLoc, ViewVelocity)));
	// DrawDebugPoint(GetWorld(), ViewLoc, 10.f, FColor::White, false, -1.f, 10);
	// DrawDebugDirectionalArrow(GetWorld(), ViewLoc, ViewLoc + ViewVelocity, 10.f, FColor::Turquoise, false, -1.f, 10, 2.f);
	// DrawDebugDirectionalArrow(GetWorld(), ViewLoc + ViewVelocity, ViewLoc + ViewVelocity + ViewDelta, 10.f, FColor::Green, false, -1.f, 10, 2.f);
	// DrawDebugPoint(GetWorld(), ViewTarget, 10.f, FColor::Yellow, false, -1.f, 10);
	// if ((ViewTarget - ViewLoc).Size() < ViewRotDistanceAhead/2)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Slowing"));
	// 	ViewVelocity *= RotBreak * DeltaTime;
	// }
	if (ViewVelocity.Size() > (ViewTarget - ViewLoc).Size() || (ViewTarget - ViewLoc).Size() < ViewRotDistanceAhead*0.25)
	{
		// ViewVelocity = FVector(0.f, 0.f, 0.f);
		ViewVelocity *= RotBreak;
		// UE_LOG(LogTemp, Warning, TEXT("Snapping"));
		ViewLoc = FMath::Lerp(ViewLoc, ViewTarget, RotBreak);
	}
	else 
	{
		ViewLoc += ViewVelocity;
	}
	// ViewLoc = FMath::Lerp(ViewLoc, ViewTarget, RotLerp);
	// DrawDebugDirectionalArrow(GetWorld(), ViewLoc, ViewVelocity, 10.f, FColor::White, false, -1.f, 10, 2.f);
	FRotator NewRot = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ViewLoc);
	// FRotator CurRot = FMath::Lerp(GetActorRotation(), NewRot, 0.1f);
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