// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Containers/Map.h"
#include "GameplayTagContainer.h"
#include "Abilities/IGetHit.h"
// #include "Props/ItemDataAsset.h"
#include "MyBlueprintFunctionLibrary.generated.h"


UENUM(BlueprintType)
enum class EInput : uint8
{
	Punch = 0,
	Kick = 1,
	Cast = 2,
	Jump = 3,
	SuperPunch = 10,
	SuperKick = 11,
	SuperCast = 12,
	SuperJump = 13,
	UltraPunch = 20,
	UltraKick = 21,
	UltraCast = 22,
	UltraJump = 23,
	Run = 101,
	Dash = 102
};

USTRUCT(BlueprintType)
struct FAbilityStruct
{
	GENERATED_BODY()

	int32 ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EInput Input = EInput::Punch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString EventName = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanUseOnGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanUseOnAir = false;

	friend bool operator==(const FAbilityStruct& first, const FAbilityStruct& second)
	{
		return first.AbilityClass == second.AbilityClass
			&& first.Input == second.Input
			&& first.EventName == second.EventName
			&& first.CanUseOnGround == second.CanUseOnGround
			&& first.CanUseOnAir == second.CanUseOnAir;
	}
	friend uint32 GetTypeHash(const FAbilityStruct& Other)
	{
		return GetTypeHash(Other.ID);
	}

};
USTRUCT(BlueprintType)
struct FMagnitudePair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag GameplayTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Magnitude = 0.f;
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


USTRUCT(BlueprintType)
struct FConditionalEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// Apply this effect if the condition is true
	FEffectContainer EffectToApply;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// Apply the effect only if have an active effect with this tag
	FGameplayTag ConditionTag;
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
	// MaxDuration > 0 caps HasDuration effects (including "forever" ones with a negative duration)
	static FActiveGameplayEffectHandle ApplyEffectContainerToChar(IGetHit* Char, FEffectContainer Container, UItemDataAsset* Item = nullptr, float MaxDuration = -1.f);

	UFUNCTION(BlueprintCallable, Category="MyLibrary")
	static TArray<FActiveGameplayEffectHandle> ApplyAllEffectContainersToActor(AActor* Actor, TArray<FEffectContainer> Containers, UItemDataAsset* Item = nullptr);
	static TArray<FActiveGameplayEffectHandle> ApplyAllEffectContainersToChar(IGetHit* Char, TArray<FEffectContainer> Containers, UItemDataAsset* Item = nullptr, float MaxDuration = -1.f);
	UFUNCTION(BlueprintCallable, Category="MyLibrary")
	static void RemoveEffectsFromActor(AActor* Actor, TArray<FActiveGameplayEffectHandle> ActiveEffects);
	UFUNCTION(BlueprintCallable, Category = "MyLibrary")
	static FActiveGameplayEffect GetActiveEffectFromHandle(UAbilitySystemComponent* GAS, const FActiveGameplayEffectHandle Handle);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MyLibrary")
	static UGameplayAbility* GetAnimatingAbilityFromActor(AActor* OnActor);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MyLibrary")
	static UGameplayAbility* GetAnimatingAbility(UAbilitySystemComponent* GAS);

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	// static TSubclassOf<class UCameraShakeBase> CamShakeClass;
};
