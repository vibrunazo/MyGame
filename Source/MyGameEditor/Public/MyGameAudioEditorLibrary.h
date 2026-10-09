#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyGameAudioEditorLibrary.generated.h"

class USoundCue;
class USoundWave;

/**
 * Editor-only audio helpers for scripts (Python commandlets, editor utilities).
 * SoundCue graph editing isn't exposed to scripting by the engine, so these keep the cue's nodes and editor graph in sync.
 */
UCLASS()
class MYGAMEEDITOR_API UMyGameAudioEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Replaces a SoundCue's whole graph with one Wave Player per wave, behind a Random node when there are several
	 * (picks without repeating until all have played). Cue-level volume/pitch multipliers and attenuation are kept.
	 */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Audio")
	static bool RebuildSoundCueFromWaves(USoundCue* Cue, const TArray<USoundWave*>& Waves, bool bLooping);
};
