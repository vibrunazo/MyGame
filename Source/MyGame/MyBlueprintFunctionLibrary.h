// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "Containers/Map.h"
#include "GameplayTagContainer.h"
#include "Abilities/AssCombatant.h"
#include "Abilities/MyGameplayEffectUIData.h"
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
	Dash = 102,
	// Not bound to a button: activated by events, AI or ability triggers
	None = 200
};

// An ability granted on a button slot. Whether it works on the ground or in the air is the ability's own
// tags (activate.groundattack / activate.flyingattack, blocked by AMyCharacter while airborne / grounded)
USTRUCT(BlueprintType)
struct FAbilityStruct
{
	GENERATED_BODY()

	int32 ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EInput Input = EInput::Punch;

	friend bool operator==(const FAbilityStruct& first, const FAbilityStruct& second)
	{
		return first.AbilityClass == second.AbilityClass
			&& first.Input == second.Input;
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
	// An ability's UI (name, description, color, icon) for the HUD's buttons and the ability books; empty when it has
	// none. The only reader of the game's ability UI table (UAbilityUIDataAsset on the game instance class)
	UFUNCTION(BlueprintPure, Category = "UI")
	static FBuffUI GetAbilityUI(TSubclassOf<UGameplayAbility> AbilityClass);

	// UFUNCTION(BlueprintCallable, Category="MyLibrary")
	// MaxDuration > 0 caps HasDuration effects (including "forever" ones with a negative duration)
	static FActiveGameplayEffectHandle ApplyEffectContainerToChar(IAssCombatant* Char, FEffectContainer Container, UItemDataAsset* Item = nullptr, float MaxDuration = -1.f);

	UFUNCTION(BlueprintCallable, Category="MyLibrary")
	static TArray<FActiveGameplayEffectHandle> ApplyAllEffectContainersToActor(AActor* Actor, TArray<FEffectContainer> Containers, UItemDataAsset* Item = nullptr);
	static TArray<FActiveGameplayEffectHandle> ApplyAllEffectContainersToChar(IAssCombatant* Char, TArray<FEffectContainer> Containers, UItemDataAsset* Item = nullptr, float MaxDuration = -1.f);
	UFUNCTION(BlueprintCallable, Category="MyLibrary")
	static void RemoveEffectsFromActor(AActor* Actor, TArray<FActiveGameplayEffectHandle> ActiveEffects);
	UFUNCTION(BlueprintCallable, Category = "MyLibrary")
	static FActiveGameplayEffect GetActiveEffectFromHandle(UAbilitySystemComponent* GAS, const FActiveGameplayEffectHandle Handle);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MyLibrary")
	static UGameplayAbility* GetAnimatingAbilityFromActor(AActor* OnActor);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MyLibrary")
	static UGameplayAbility* GetAnimatingAbility(UAbilitySystemComponent* GAS);
#if WITH_EDITOR
	// PIE test hook: seeds the engine's global random (FMath::Rand/FRand and SRand, on the calling thread, the game
	// thread) and the AI system's own stream (EQS random picks and tests), so a scenario's AI draws the same numbers
	UFUNCTION(BlueprintCallable, Category = "Testing")
	static void SeedRandomForTest(int32 Seed);
	// PIE test hook: lifts the AI's wall-clock budgets in this world (EQS testing time per tick, sight's time slice), so
	// a query or a sight check never finishes a frame later because the machine was busy. Returns what it set
	UFUNCTION(BlueprintCallable, Category = "Testing", meta = (WorldContext = "WorldContext"))
	static FString LiftAIBudgetsForTest(UObject* WorldContext);
	// PIE test hook: the AI system's stream's current seed (read without drawing)
	UFUNCTION(BlueprintCallable, Category = "Testing")
	static int32 GetAIRandomSeedForTest();
	// PIE test hook: a combatant's landed stuns counted towards stun immunity (they never decay) go back to none, so a
	// test scenario starts from none. Returns the count before. Takes the actor, wherever its combat state lives
	UFUNCTION(BlueprintCallable, Category = "Testing")
	static int32 ResetHitStunCountForTest(AActor* Combatant);
	// PIE test hook: from the next world tick on, every tick of WorldContext's world starts by seeding the randoms
	// (SeedRandomForTest) from Seed and the tick's number, so a tick's draws never depend on how many draws came
	// before it (other code draws from the same game-thread random between ticks, a varying number of times).
	// A Seed below 0 stops it
	UFUNCTION(BlueprintCallable, Category = "Testing", meta = (WorldContext = "WorldContext"))
	static void SeedEveryTickForTest(UObject* WorldContext, int32 Seed);
#endif

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	// static TSubclassOf<class UCameraShakeBase> CamShakeClass;
};
