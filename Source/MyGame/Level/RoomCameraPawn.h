// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RoomCameraPawn.generated.h"

UCLASS()
class MYGAME_API ARoomCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARoomCameraPawn();
	virtual void Tick(float DeltaTime) override;
	UFUNCTION(BlueprintCallable, Category = RoomCamera)
	void SetPlayerRef(class AMyCharacter* NewPlayer);

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
	FVector ViewTarget = FVector();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float RotLerp = 0.05f;
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

private:
	void FollowPlayer();
	FVector GetRoomDistance(); 
	FVector GetRoomSize(); 

	class ALevelBuilder* LevelBuilderRef = nullptr;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
