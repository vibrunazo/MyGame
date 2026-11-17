// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AssCombatComponent.h"
#include "AssCombatLibrary.h"
#include "MyGameInstance.h"
#include "UI/AbilityUIDataAsset.h"
#include "GameMapsSettings.h"
#include "Player/MyCharacter.h"
#include "AbilitySystemComponent.h"
#include "Props/ItemDataAsset.h"

#include "AbilitySystemInterface.h"
#include "AISystem.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Perception/AISense_Sight.h"
#include "UObject/UObjectIterator.h"

#if WITH_EDITOR
void UMyBlueprintFunctionLibrary::SeedRandomForTest(int32 Seed)
{
	FMath::RandInit(Seed);
	FMath::SRandInit(Seed);
	UAISystem::SeedRandomStream(Seed);
}

FString UMyBlueprintFunctionLibrary::LiftAIBudgetsForTest(UObject* WorldContext)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World) return TEXT("no world");
	constexpr double Lifted = 1000.0;
	FString Out;
	if (UEnvQueryManager* EQS = UEnvQueryManager::GetCurrent(World))
	{
		if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(UEnvQueryManager::StaticClass(), TEXT("MaxAllowedTestingTime")))
		{
			Prop->SetPropertyValue_InContainer(EQS, (float)Lifted);
			Out += TEXT("EQS MaxAllowedTestingTime; ");
		}
	}
	ForEachObjectOfClass(UAISense_Sight::StaticClass(), [&](UObject* Sense)
	{
		if (Sense->GetWorld() != World) return;
		if (FDoubleProperty* Prop = FindFProperty<FDoubleProperty>(UAISense_Sight::StaticClass(), TEXT("MaxTimeSlicePerTick")))
		{
			Prop->SetPropertyValue_InContainer(Sense, Lifted);
			Out += TEXT("sight MaxTimeSlicePerTick; ");
		}
	});
	return Out;
}

int32 UMyBlueprintFunctionLibrary::GetAIRandomSeedForTest()
{
	return UAISystem::GetRandomStream().GetCurrentSeed();
}

namespace
{
	struct FTickSeed
	{
		FDelegateHandle Handle;
		TWeakObjectPtr<UWorld> World;
		int32 Seed = -1;
		int32 Tick = 0;
	};
	FTickSeed GTickSeed;
}

void UMyBlueprintFunctionLibrary::SeedEveryTickForTest(UObject* WorldContext, int32 Seed)
{
	if (GTickSeed.Handle.IsValid()) FWorldDelegates::OnWorldTickStart.Remove(GTickSeed.Handle);
	GTickSeed = FTickSeed();
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (Seed < 0 || !World) return;
	GTickSeed.World = World;
	GTickSeed.Seed = Seed;
	GTickSeed.Handle = FWorldDelegates::OnWorldTickStart.AddLambda([](UWorld* Ticking, ELevelTick, float)
	{
		if (Ticking != GTickSeed.World.Get()) return;
		SeedRandomForTest((int32)HashCombine(GetTypeHash(GTickSeed.Seed), GetTypeHash(++GTickSeed.Tick)));
	});
}

int32 UMyBlueprintFunctionLibrary::ResetHitStunCountForTest(AActor* Combatant)
{
	UAssCombatComponent* Combat = UAssCombatComponent::Find(Combatant);
	return Combat ? Combat->ResetHitStunCount() : 0;
}
#endif


// Items' effects: an item with a cue adds it in the effect's context, then the plugin applies the effect
FActiveGameplayEffectHandle UMyBlueprintFunctionLibrary::ApplyEffectContainerToChar(IAssCombatant* Char, const FAssEffectContainer& Container, UItemDataAsset* Item)
{
    AActor* Actor = Cast<AActor>(Cast<UObject>(Char));
    UAbilitySystemComponent* GAS = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
    if (!ensure(GAS != nullptr)) return FActiveGameplayEffectHandle();
    if (!Container.EffectClass) return FActiveGameplayEffectHandle();
    FGameplayEffectContextHandle Context = GAS->MakeEffectContext();
    if (Item && Item->CueTag.ToString() != "")
    {
        GAS->AddGameplayCue(Item->CueTag, Context);
        UE_LOG(LogTemp, Warning, TEXT("Added %s to Context"), *Item->CueTag.ToString());
    }
    return UAssCombatLibrary::ApplyEffectContainer(Actor, Container, -1.f, Context);
}
void UMyBlueprintFunctionLibrary::RemoveEffectsFromActor(AActor* Actor, TArray<FActiveGameplayEffectHandle> ActiveEffects)
{
    UAbilitySystemComponent* GAS = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
    if (!GAS) return;
    for (auto &&Effect : ActiveEffects)
    {
        GAS->RemoveActiveGameplayEffect(Effect);
    }
    
}
FActiveGameplayEffect UMyBlueprintFunctionLibrary::GetActiveEffectFromHandle(UAbilitySystemComponent* GAS, const FActiveGameplayEffectHandle Handle)
{
    auto we = GAS->GetActiveGameplayEffect(Handle);
    return *we;
}
UGameplayAbility* UMyBlueprintFunctionLibrary::GetAnimatingAbilityFromActor(AActor* OnActor)
{
    if (!OnActor) return nullptr;
    IAbilitySystemInterface* GASInterface = Cast<IAbilitySystemInterface>(OnActor);
    if (!GASInterface) return nullptr;
    return GetAnimatingAbility(GASInterface->GetAbilitySystemComponent());
}
UGameplayAbility* UMyBlueprintFunctionLibrary::GetAnimatingAbility(UAbilitySystemComponent* GAS)
{
    if (!GAS) return nullptr;
    return GAS->GetAnimatingAbility();
}
TArray<FActiveGameplayEffectHandle> UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToActor(AActor* Actor, TArray<FAssEffectContainer> Containers, UItemDataAsset* Item)
{
    IAssCombatant* Char = Cast<IAssCombatant>(Actor);
    if (!Char) return {};
    return ApplyAllEffectContainersToChar(Char, Containers, Item);
}

TArray<FActiveGameplayEffectHandle> UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToChar(IAssCombatant* Char, const TArray<FAssEffectContainer>& Containers, UItemDataAsset* Item)
{
    TArray<FActiveGameplayEffectHandle> result = {};
    for (auto &&Container : Containers)
    {
        FActiveGameplayEffectHandle NewActiveEffect = ApplyEffectContainerToChar(Char, Container, Item);
        result.Add(NewActiveEffect);
    }
    return result;
}
/** Returns const pointer to the actual active gamepay effect structure */
//const FActiveGameplayEffect* GetActiveGameplayEffect(const FActiveGameplayEffectHandle Handle) const;
FBuffUI UMyBlueprintFunctionLibrary::GetAbilityUI(TSubclassOf<UGameplayAbility> AbilityClass)
{
	// from the game instance class's defaults, so it answers in the editor too (a book's construction script)
	const UClass* GameInstanceClass = GetDefault<UGameMapsSettings>()->GameInstanceClass.TryLoadClass<UMyGameInstance>();
	const UMyGameInstance* GameInstance = GameInstanceClass ? GameInstanceClass->GetDefaultObject<UMyGameInstance>() : nullptr;
	const FBuffUI* Found = GameInstance && GameInstance->AbilityUI && AbilityClass ? GameInstance->AbilityUI->Abilities.Find(AbilityClass) : nullptr;
	return Found ? *Found : FBuffUI();
}
