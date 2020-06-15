// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MyAIController.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API AMyAIController : public AAIController
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My AI Controller")
	float AttackRange = 140.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My AI Controller")
	float ChaseRange = 40.f;
};
