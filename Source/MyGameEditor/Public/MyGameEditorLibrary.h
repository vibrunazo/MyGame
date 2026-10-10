#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyGameEditorLibrary.generated.h"

class USoundCue;
class USoundWave;

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

};
