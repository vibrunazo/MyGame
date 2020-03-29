// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelBuilder.generated.h"

UCLASS()
class MYGAME_API ALevelBuilder : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALevelBuilder();

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LevelBuilder)
	class UBillboardComponent* BBComp;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


};
