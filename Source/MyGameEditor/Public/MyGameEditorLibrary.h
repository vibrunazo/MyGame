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
class UAnimationAsset;
class UAnimSequence;
class UAnimBlueprint;
class USkeletalMesh;
class USkeleton;

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

	/**
	 * Moves an animation asset to another skeleton in place, so its path, its notifies and everything referring to it
	 * stay (the move to the UE5 mannequin, Step 0c). A sequence takes its tracks and curves from Retargeted, a copy the
	 * IK retargeter made of it on the new skeleton; a montage, blend space or pose asset only changes skeleton (its
	 * sequences are moved themselves). The asset is left dirty, for the caller to save.
	 */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Animation")
	static bool MoveAnimationToSkeleton(UAnimationAsset* Asset, USkeletalMesh* Mesh, UAnimSequence* Retargeted);

	/** Points an animation Blueprint at another skeleton in place, refreshes its nodes and compiles it. */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Animation")
	static bool MoveAnimBlueprintToSkeleton(UAnimBlueprint* Blueprint, USkeletalMesh* Mesh);

	/**
	 * Where a hit window samples Bone at montage time Position (the montage's skeleton's component space): the game's
	 * own sampling (AAssHitBox::MontageBoneInComponent). Identity and false when the montage plays no animation data there.
	 */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Animation")
	static bool MontageBoneForTest(UAnimMontage* Montage, FName Bone, float Position, FTransform& Transform);

	/** Adds a socket to a skeleton, or sets it when the skeleton has one by that name (a socket's names are read-only to scripts). */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Animation")
	static bool SetSkeletonSocket(USkeleton* Skeleton, FName Socket, FName Bone, const FTransform& Local);

	/**
	 * Ass Step 7's world check (Docs/Step6.md 4.8): the plugin's spot search against the engine's spot query, in this
	 * world, for Querier with Target as its target. The engine runs a copy of Query in memory with its scores left as the
	 * query's best-25% pick sees them (its All Matching mode would rescale them), and its band is the items scoring at
	 * least Ratio of the best. The plugin's band is UAssMoverComponent::SpotPoints scored by AssSpots::Band, the Spot
	 * fields given here. JSON {"engine": [[x, y, z, score]...], "plugin": [[x, y, z, score, sight, distance]...],
	 * "engine_points", "plugin_points", "only_engine", "only_plugin"}: each band, and the points of one band missing from
	 * the other (to the centimetre). "" when the inputs are missing.
	 */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing")
	static FString CompareSpotSearchForTest(APawn* Querier, ACharacter* Target, class UEnvQuery* Query, float Behind,
		float HalfExtent, float Spacing, float DistanceWeight, float SightScore, float Ratio);

	/**
	 * Ass Step 7's world check: puts a token arbiter (UAssTokenArbiterComponent, one token, kept until death) on Player
	 * beside its aggro list, and from now on asks it every question the list answers (AMyCharacter::AggroListProbe):
	 * an addition is a request, IsInAggroList is Held Or Free. A shadow already running is dropped.
	 */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing")
	static bool StartTokenShadowForTest(ACharacter* Player);

	/**
	 * Test setup for enemies on patterns (Ass Step 7): Pawn takes Target's attack token (its UAssTokenArbiterComponent),
	 * as the tests' aggro_add puts an enemy on the aggro list. False when refused or Target has no arbiter.
	 */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing")
	static bool RequestTokenForTest(AActor* Target, AActor* Pawn);

	/** Test setup: Target's arbiter holds no token (every character in the world lets go), as clear_aggro_list empties the list. */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing")
	static bool ClearTokensForTest(AActor* Target);

	/** Stops the shadow, removes its arbiter, and returns JSON {"questions", "additions", "added", "mismatches", "removed_holder"}: the answers that differ only because a holder was removed without dying (the list keeps it until garbage collection, the arbiter lets it go) are apart. */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Testing")
	static FString StopTokenShadowForTest();

};
