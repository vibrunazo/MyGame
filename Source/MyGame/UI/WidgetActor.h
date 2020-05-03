// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WidgetActor.generated.h"

UCLASS()
class MYGAME_API AWidgetActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWidgetActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class USceneComponent* MyRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	class UWidgetComponent* WidgetComp;

	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetWidgetText(class UItemDataAsset* NewItem);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


};
