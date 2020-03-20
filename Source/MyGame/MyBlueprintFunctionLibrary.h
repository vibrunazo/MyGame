// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"


USTRUCT(BlueprintType)
struct FAbilityStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Input;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanUseOnGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanUseOnAir = false;

};

/**
 * 
 */
UCLASS()
class MYGAME_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
};
