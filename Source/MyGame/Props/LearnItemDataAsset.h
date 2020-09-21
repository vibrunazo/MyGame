// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemDataAsset.h"
#include "LearnItemDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API ULearnItemDataAsset : public UItemDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TArray<struct FAbilityStruct> AbilitiesToLearn;
	
};
