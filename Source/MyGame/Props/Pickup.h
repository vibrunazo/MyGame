// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "Components/TimelineComponent.h"
#include "Pickup.generated.h"


UCLASS()
class MYGAME_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickup();
	void OnConstruction(const FTransform & Transform) override;
	UFUNCTION()
	void OnPickupBeginOverlap(AActor* OverlappingActor, AActor* OtherActor);
	void OnDelayedSpawn();
	UFUNCTION()
	void OnTimelineCallback();
	void OnTimelineUpdate();
	void EnablePickup();
	void ApplyItemData();
	void UpdateFromItemData(class UItemDataAsset* NewItemData);
	void SetItemData(class UItemDataAsset* NewItemData);
	class APickupMeshActor* GetPickupMeshActor();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Item Pickup")
	class APickupMeshActor* BP_GetChildActorRef();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Item Pickup")
	void BP_UpdateChildActor(TSubclassOf<class APickupMeshActor> NewMeshActorClass);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Item Pickup")
	class ULootComponent* LootComponent;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Item Pickup")
	class USceneComponent* RootComp;
	/*UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Item Pickup")
	class UStaticMeshComponent* Mesh;*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Pickup")
	class UBoxComponent* BoxCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Pickup")
	class UBoxComponent* BoxTrigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	TSubclassOf<class APickupMeshActor> MeshActorClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class UItemDataAsset* ItemData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class UNiagaraSystem* SpawnParticles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class USoundBase* SpawnSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class USoundBase* PickupSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	class UNiagaraSystem* PickupParticles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	bool bMaxHPCanPickup = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	uint8 TeamWhoCanPickup = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup", meta = (ToolTip = "Time after the Item spawns that it becomes visible and starts spawn animation. Varies randomly between Min and Max."))
	float DelayedSpawnMin = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup", meta = (ToolTip = "Time after the Item spawns that it becomes visible and starts spawn animation. Varies randomly between Min and Max."))
	float DelayedSpawnMax = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup", meta = (ToolTip = "Time after the Item delayed spawn that it takes to enable pickup by the player."))
	float PickupTime = 0.7f;
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Pickup")
	TSubclassOf<class AWidgetActor> WidgetActorClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	UCurveFloat *CurveScale;
	FTimeline MyTimeline;
	FTimerHandle TimelineTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	bool bUseRandomPool = false;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Pickup")
	// TArray<class UItemDataAsset*> RandomPool = {};

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY()
	class APickupMeshActor* APickupMeshActorRef = nullptr;
};