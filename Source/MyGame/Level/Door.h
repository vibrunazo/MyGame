// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RoomDataAsset.h"
#include "GameFramework/Actor.h"
#include "Door.generated.h"

UCLASS()
class MYGAME_API ADoor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADoor();

	UFUNCTION(BlueprintCallable, Category = Door)
	void OpenDoor();
	UFUNCTION(BlueprintImplementableEvent, Category = Door)
	void OnOpenDoorBP();
	UFUNCTION(BlueprintCallable, Category = Door)
	void CloseDoor();
	UFUNCTION(BlueprintImplementableEvent, Category = Door)
	void OnCloseDoorBP();
	void SetSymbol(ERoomType SymbolType);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Door)
	class USceneComponent* MyRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Door)
	class UBoxComponent* BoxCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Door)
	class UStaticMeshComponent* DoorMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Door)
	class UStaticMeshComponent* DoorFrame;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Door)
	class UStaticMeshComponent* SymbolMeshA;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Door)
	class UStaticMeshComponent* SymbolMeshB;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Door)
	class UStaticMesh *TreasureMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Door)
	class UNiagaraSystem* TreasureParticles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Door)
	class UStaticMesh *BossMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Door)
	class UNiagaraSystem* BossParticles;

	bool bIsDoorOpen = false;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
