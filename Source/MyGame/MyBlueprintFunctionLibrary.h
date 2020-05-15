// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Containers/Map.h"
#include "GameplayTagContainer.h"
#include "Abilities/IGetHit.h"
// #include "Props/ItemDataAsset.h"
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
	FString EventName = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanUseOnGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanUseOnAir = false;

};
USTRUCT(BlueprintType)
struct FMagnitudePair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag GameplayTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Magnitude;
};

USTRUCT(BlueprintType)
struct FEffectContainer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FMagnitudePair> Magnitudes;
};


/**
 * 
 */
UCLASS(Blueprintable)
class MYGAME_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// UFUNCTION(BlueprintCallable, Category="MyLibrary")
	static FActiveGameplayEffectHandle ApplyEffectContainerToChar(IGetHit* Char, FEffectContainer Container, UItemDataAsset* Item = nullptr);

	UFUNCTION(BlueprintCallable, Category="MyLibrary")
	static TArray<FActiveGameplayEffectHandle> ApplyAllEffectContainersToActor(AActor* Actor, TArray<FEffectContainer> Containers, UItemDataAsset* Item = nullptr);
	static TArray<FActiveGameplayEffectHandle> ApplyAllEffectContainersToChar(IGetHit* Char, TArray<FEffectContainer> Containers, UItemDataAsset* Item = nullptr);
	UFUNCTION(BlueprintCallable, Category="MyLibrary")
	static void RemoveEffectsFromActor(AActor* Actor, TArray<FActiveGameplayEffectHandle> ActiveEffects);

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	// static TSubclassOf<class UCameraShake> CamShakeClass;
};
