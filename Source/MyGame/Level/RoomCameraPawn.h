// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LevelBuilder.h"
#include "GameFramework/Pawn.h"
#include "RoomCameraPawn.generated.h"

UCLASS()
class MYGAME_API ARoomCameraPawn : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARoomCameraPawn();
	virtual void Tick(float DeltaTime) override;
	UFUNCTION(BlueprintCallable, Category = RoomCamera)
	void SetPlayerRef(class AMyCharacter* NewPlayer);
	FVector GetCameraLoc();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Camera)
	class USceneComponent* RootComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UCameraComponent* FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UProjectileMovementComponent* MovementComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	class AMyCharacter* PlayerRef;
	FVector Target = FVector();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	FVector CameraDistance = FVector(-800.f, 0.f, 300.f);
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	// float XRatio = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float LerpSpeed = 0.05f;
	FVector ViewLoc = FVector();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float RotSpeed = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float RotInterpExp = 1.f;
	// How long I've been in the current direction. Will be positive if the direction is to the right, negative to the left
	float TimeInCurrentDirection = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float DelayBeforeStartsMoving = 0.5f;
	// Point to start the Lerp that will change the Rotation, will reset every time player changes direction
	FVector RotSource = FVector();
	float RotLerpAlpha = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	// Offset from the player the camera Rotation should be looking at
	float RotOffset = 400.f;
	float CurViewDistance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (ClampMin = "0", ClampMax = "1"))
	// How far into the room to clamp how far the camera rotation can point to, when the player is looking at a wall. 0 means center of the room, 1 means in the wall
	float RotWallClamp = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float MaxXRatio = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float MinXRatio = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float YRatio = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float MaxFoV = 50.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float MinFoV = 90.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float FoVLerp = 0.05f;
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	bool bDrawDebugLines = false;*/

	// How far top the camera can go from the center of the room, when there's a wall in the direction the player is facing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float WallClampTop = 200.f;
	// How far bottom the camera can go from the center of the room, when there's a wall in the direction the player is facing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float WallClampBottom = 300.f;
	// How far right the camera can go from the center of the room, when there's a wall in the direction the player is facing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float WallClampRight = 400.f;
	// How far left the camera can go from the center of the room, when there's a wall in the direction the player is facing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float WallClampLeft = 400.f;
	// How much the offset accelerates 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float OffsetAccel = 20.f;

private:
	void FollowPlayer(float DeltaTime);
	void FollowPlayer2(float DeltaTime);
	FVector GetRoomDistance(); 
	FVector GetRoomSize(); 
	class UMyGameInstance* GetMyGameInstance();
	class ALevelBuilder* GetLevelBuilder();
	void TryRegisterEnterRoomEvent();
	UFUNCTION()
	void OnEnterRoom(struct FRoomState NewRoom);

	UPROPERTY()
	class UMyGameInstance* MyGameInstanceRef = nullptr;
	UPROPERTY()
	class ALevelBuilder* LevelBuilderRef = nullptr;
	bool bIsRoomEnterRegistered = false;
	struct FRoomState* CurrentRoomRef = nullptr;
	bool bIsWalledLeft = false;
	bool bIsWalledRight = false;
	bool bIsWalledTop = false;
	bool bIsWalledBottom = false;
	// A Vector to decide if the offset should change or not. Will increase everytime the player is looking at the same direction.
	// Its length is how long in seconds the player is looking at that direction
	FVector OffsetTimeVector = FVector();
	// The actual offset of the camera ahead from the Player Character
	FVector CurrentOffset = FVector();
	FVector CurrentRoomSize;
	FVector CurrentRoomPosition;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
