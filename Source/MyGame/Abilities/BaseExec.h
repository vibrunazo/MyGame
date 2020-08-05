// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "BaseExec.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UBaseExec : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	//UBaseExec();
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
	
};
