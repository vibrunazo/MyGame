#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyGameEditorLibrary.generated.h"

class USoundCue;
class USoundWave;
class UAnimMontage;

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

};
