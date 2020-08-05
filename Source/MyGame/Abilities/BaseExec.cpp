// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseExec.h"

void UBaseExec::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UE_LOG(LogTemp, Warning, TEXT("base exec"));
}
