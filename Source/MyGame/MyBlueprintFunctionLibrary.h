// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "Containers/Map.h"
#include "GameplayTagContainer.h"
#include "AssCombatant.h"
#include "AssEffectContainer.h"
#include "AssAbilityTemplate.h"
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

// MyGame's abilities are grants of the plugin's templates (FAssAbilityGrant, Ass Plan.md Step 4): a grant's slot key is
// EInput's number, and None is FAssAbilityGrant::NoSlot. Whether a move works on the ground or in the air is its
// definition's identity tags (activate.groundattack / activate.flyingattack, blocked by AMyCharacter while airborne /
// grounded).
/**
 * 
 */
UCLASS(Blueprintable)
class MYGAME_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// An ability's UI for the HUD's buttons and the ability books, from its template's presentation: its name and
	// description, white, and its icon texture as a 64 x 64 brush (what DA_AbilityUI held for every entry); empty with
	// no template, and a default brush with no icon
	UFUNCTION(BlueprintPure, Category = "UI")
	static FBuffUI GetAbilityUI(const UAssAbilityTemplate* Template);
	UFUNCTION(BlueprintPure, Category = "UI")
	static FBuffUI GetGrantUI(const FAssAbilityGrant& Grant);
	// A grant's template, loaded; nullptr with none
	UFUNCTION(BlueprintPure, Category = "Abilities")
	static UAssAbilityTemplate* GetGrantTemplate(const FAssAbilityGrant& Grant);
	// A grant's slot key as the input it's on (NoSlot is None), and back
	UFUNCTION(BlueprintPure, Category = "Abilities")
	static EInput GetGrantInput(const FAssAbilityGrant& Grant);
	static int32 SlotKeyOf(EInput Input) { return Input == EInput::None ? FAssAbilityGrant::NoSlot : (int32)Input; }

	// The plugin's UAssCombatLibrary::ApplyEffectContainer, with an item's cue added in the effect's context first
	static FActiveGameplayEffectHandle ApplyEffectContainerToChar(IAssCombatant* Char, const FAssEffectContainer& Container, UItemDataAsset* Item = nullptr);

	UFUNCTION(BlueprintCallable, Category="MyLibrary")
	static TArray<FActiveGameplayEffectHandle> ApplyAllEffectContainersToActor(AActor* Actor, TArray<FAssEffectContainer> Containers, UItemDataAsset* Item = nullptr);
	static TArray<FActiveGameplayEffectHandle> ApplyAllEffectContainersToChar(IAssCombatant* Char, const TArray<FAssEffectContainer>& Containers, UItemDataAsset* Item = nullptr);
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
	// PIE test hook: the pattern stepper as in a new world of a run with RunSeed (AMyPatternStepper::ResetPlaces): the
	// enemies a scenario spawns next get a fresh room's salts, so a seed repeats and another seed draws differently
	UFUNCTION(BlueprintCallable, Category = "Testing", meta = (WorldContext = "WorldContext"))
	static void ResetPatternPlacesForTest(UObject* WorldContext, int32 RunSeed);
#endif

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	// static TSubclassOf<class UCameraShakeBase> CamShakeClass;
};
