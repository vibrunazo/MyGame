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
	void EnablePickup();

	// UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Item Pickup")
	// class USceneComponent* RootComp;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Item Pickup")
	class UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Pickup")
	class UBoxComponent* BoxCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Pickup")
	class UBoxComponent* BoxTrigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class UItemDataAsset* ItemData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	bool bMaxHPCanPickup = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	uint8 TeamWhoCanPickup = 0;
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Pickup")
	TSubclassOf<class AWidgetActor> WidgetActorClass;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};


USTRUCT(BlueprintType)
struct FLootDrop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<APickup> Pickup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 DropRate;
};