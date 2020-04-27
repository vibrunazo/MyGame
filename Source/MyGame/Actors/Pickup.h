// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "Pickup.generated.h"

UCLASS()
class MYGAME_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickup();

	UFUNCTION()
	void OnPickupBeginOverlap(AActor* OverlappingActor, AActor* OtherActor);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Pickup)
	class USceneComponent* RootComp;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Pickup)
	class UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pickup)
	class UBoxComponent* BoxCollision;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pickup)
	TArray<FEffectContainer> EffectsToApply;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
