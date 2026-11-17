#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "MyGameEditorLibrary.generated.h"

class USoundCue;
class USoundWave;
class UAnimMontage;
class UGameplayEffect;

/**
 * Editor-only helpers for scripts (Python commandlets, editor utilities) where the engine exposes no scripting API.
 */
UCLASS()
class MYGAMEEDITOR_API UMyGameEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * SoundCue graphs can't be edited from scripts. Replaces a SoundCue's whole graph with one Wave Player per wave, behind a Random node when there are several
	 * (picks without repeating until all have played), keeping the editor graph in sync. Cue-level volume/pitch
	 * multipliers and attenuation are kept.
	 */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Audio")
	static bool RebuildSoundCueFromWaves(USoundCue* Cue, const TArray<USoundWave*>& Waves, bool bLooping);

	/**
	 * A montage's composite sections, which scripts can't read: a JSON array of {"name", "start", "next"} in the
	 * montage's order (start in seconds of montage time; next is the section it links to, "None" when it ends there).
	 */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Dump")
	static FString DescribeMontageSections(UAnimMontage* Montage);

	/** The objects whose outer is Object (one level), by path: an asset's instanced subobjects that scripts can't list. */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Dump")
	static TArray<FString> ListSubobjects(UObject* Object);

	/**
	 * Starts the strict recorder (UCombatRecorder) in the world of WorldContext, following each actor under its role
	 * name (Roles and Actors pair up). A recording already running is stopped and dropped.
	 */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing", meta = (WorldContext = "WorldContext"))
	static bool StartCombatRecord(UObject* WorldContext, const TArray<FString>& Roles, const TArray<AActor*>& Actors);

	/**
	 * Test only (the strict recorder's deliberate breaks): replaces the tags a gameplay effect's Target Tags component
	 * adds to its target, in memory, and returns the ones it added before. Never saved.
	 */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing")
	static FGameplayTagContainer SetEffectGrantedTagsForTest(TSubclassOf<UGameplayEffect> Effect, const FGameplayTagContainer& Added);

	/** Test only: a property's value as the editor's text (what copy and paste use), "" when the object has no such property. */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing")
	static FString GetPropertyTextForTest(UObject* Object, FName Property);

	/** Test only: sets a property from the editor's text, in memory, on a default or an instance alike. Never saved. */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing")
	static bool SetPropertyTextForTest(UObject* Object, FName Property, const FString& Text);

	/** The cancel matrix's query: these roles of the running recording add, per frame, the abilities they could start. */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing")
	static void QueryCombatRecordRoles(const TArray<FString>& Roles);

	/** Stops the strict recorder and returns its record as JSON ("" when none ran). */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing")
	static FString StopCombatRecord();

};
