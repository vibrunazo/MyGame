// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomCameraPawn.h"
#include "../Player/MyCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"

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
		FollowPlayer();
	}

}

void ARoomCameraPawn::SetPlayerRef(AMyCharacter* NewPlayer)
{
	PlayerRef = NewPlayer;
	// MovementComponent->HomingTargetComponent = PlayerRef->GetRootComponent();
}

void ARoomCameraPawn::FollowPlayer()
{
	// FollowCamera->RelativeRotation(NewRot);
	float TargetX = PlayerRef->GetActorLocation().X ;
	// Target = PlayerRef->GetActorLocation()  + FVector(-800.f - TargetX * 0.8, 0.f, 300.f);
	Target = PlayerRef->GetActorLocation()  + CameraDistance;
	Target.X -= TargetX * XRatio;
	GetActorLocation();
	FVector CurLoc = FMath::Lerp(GetActorLocation(), Target, LerpSpeed);
	// CurLoc += FVector(-500.f, 0.f, 400.f);

	SetActorLocation(CurLoc);

	ViewTarget = FMath::Lerp(ViewTarget, PlayerRef->GetActorLocation(), RotLerp);
	FRotator NewRot = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ViewTarget);
	// FRotator CurRot = FMath::Lerp(GetActorRotation(), NewRot, 0.1f);
	SetActorRotation(NewRot);
	// SetActorLocation(FVector(GetActorLocation().X, Target.Y, GetActorLocation().Z));

}