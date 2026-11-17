#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Dom/JsonValue.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "CombatRecorder.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
struct FGameplayEffectSpec;
struct FActiveGameplayEffect;
struct FAbilityEndedData;
struct FTestProbeEvent;
enum class EGameplayCueExecutionOptions : int32;

/**
 * The strict recorder (Ass Plan.md, Step 0c item 2): everything the named characters do in a play world, frame by
 * frame and event by event, recorded inside the engine so nothing falls between two samples. Started and stopped by
 * the tests (UMyGameEditorLibrary::StartCombatRecord / StopCombatRecord); the record is JSON:
 *
 *   roles   {role: class path}: the characters named at the start, then each actor that spawns while recording, named
 *           by its class and order ("BP_Projectile_C#1"); a record never names an instance
 *   frames  {role: [[frame, t, x, y, z, pitch, yaw, roll, montage, section, position, dilation, movement mode], ...]}
 *           after every frame's actors ticked (montage is its package path, "" when none); a queried role's rows end
 *           with [the ids of its granted abilities that could start now], each asked as a press would ask it
 *           (CanActivateAbility on its primary instance, starting nothing)
 *   events  [[frame, t, kind, role, ...], ...] in the order they happened:
 *           ability   role, "activated" | "ended" | "cancelled", ability id
 *           failed    role, ability id, [the tags that blocked it]
 *           effect    role (target), effect id, instigator role, causer role, source ability id, level, duration,
 *                     period, [[set-by-caller tag, magnitude], ...] sorted, stack count
 *           periodic  the same, each time a periodic effect executes
 *           removed   role, effect id
 *           tag       role, tag, new count
 *           attribute role, attribute, old value, new value
 *           cue       role (target), tag, event (0 OnActive, 1 WhileActive, 2 Executed, 3 Removed), raw magnitude,
 *                     normalized magnitude, location in the target's actor space
 *           anim      role, "fired" | "begin" | "end", the anim notify's name, its class (a montage's branching-point
 *                     notifies, which the engine runs from the montage and not from the queue, are taken from the
 *                     montage's position as the engine takes them: a state while the position is inside it, a
 *                     notify when the position passes it)
 *           spawn     role (the new actor's), class path, owner role, instigator role, location (not a cue's actor,
 *                     which comes from a pool)
 *           probe     role, what, other role, detail, [x, y, z], number (Source/MyGame/Dev/TestProbe.h: a hit shape's
 *                     centre in the attacker's actor space at a sample, a hit, an effect tried, a projectile refused,
 *                     a backslide, a camera shake, a sound or particles played from C++)
 *
 * The recorder also fixes the order the roles tick in, which the engine leaves to chance between sessions (a hit
 * starts the victim's reaction while the attacker ticks, and whether the victim's mesh has ticked yet that frame moved
 * its montage by a frame): within each tick group, every tick function of a role and its controller runs after those
 * of the roles named before it (the ones named at the start in their order, then each that spawns), and a role's own
 * run in one fixed order (the engine's prerequisites kept). The order goes when it stops.
 *
 * An ability or effect id is its asset's package path (/Game/Abilities/GA_Punch), the name the templates of Step 3
 * take from folder and name; a native class gives its class path. A frame's number counts the world's ticks since the
 * start, and an event between two ticks (a test's input) carries the last one's; times add up the ticks' steps (so they
 * don't depend on the world's clock when the recording started). A hit's number is how long before its frame's end
 * its sample was, in seconds to the microsecond (0 or less).
 */
UCLASS()
class MYGAMEEDITOR_API UCombatRecorder : public UObject
{
	GENERATED_BODY()

public:
	static bool Start(UWorld* World, const TArray<FString>& Roles, const TArray<AActor*>& Actors);
	static FString Stop();
	static bool IsRecording();
	/** The cancel matrix's query: from now on these roles' frame rows end with the abilities they could start. */
	static void QueryRoles(const TArray<FString>& Names);

private:
	struct FRole
	{
		FString Name;
		TWeakObjectPtr<AActor> Actor;
		TWeakObjectPtr<UAbilitySystemComponent> ASC;
		TArray<TPair<FGameplayAttribute, FDelegateHandle>> AttributeHandles;
		FDelegateHandle Applied, Periodic, Removed, Tags, Activated, Ended, Failed;
		TSet<FString> ActiveStates;
		bool bQuery = false;
		TWeakObjectPtr<const class UAnimMontage> LastMontage;
		float LastPosition = -1.f;
		TArray<TSharedPtr<FJsonValue>> Frames;
	};

	void Begin(UWorld* InWorld);
	void AddRole(const FString& Name, AActor* Actor);
	void TickAfterEarlierRoles(AActor* Actor);
	void Unbind(FRole& Role);
	FString RoleOf(const UObject* Object);
	void Event(const FString& Kind, const FString& Role, TArray<TSharedPtr<FJsonValue>> Fields);
	double Now() const;

	void OnTickStart(UWorld* InWorld, ELevelTick TickType, float DeltaSeconds);
	void OnPostActorTick(UWorld* InWorld, ELevelTick TickType, float DeltaSeconds);
	void OnSpawned(AActor* Actor);
	void OnEffect(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle, int32 Role, bool bPeriodic);
	void OnEffectRemoved(const FActiveGameplayEffect& Effect, int32 Role);
	void OnTag(const FGameplayTag Tag, int32 Count, int32 Role);
	void OnAttribute(const FOnAttributeChangeData& Data, int32 Role);
	void OnActivated(UGameplayAbility* Ability, int32 Role);
	void OnEnded(const FAbilityEndedData& Data, int32 Role);
	void OnFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& Tags, int32 Role);
	void OnCue(AActor* Target, FGameplayTag Tag, EGameplayCueEvent::Type Type, const FGameplayCueParameters& Parameters, EGameplayCueExecutionOptions Options);
	void OnProbe(const FTestProbeEvent& Probe);

	TWeakObjectPtr<UWorld> World;
	double Elapsed = 0.0;
	int32 Frame = 0;
	TArray<FRole> Roles;
	TMap<TWeakObjectPtr<const UObject>, FString> Names;
	TMap<FString, int32> ClassCounts;
	TArray<TSharedPtr<FJsonValue>> Events;
	struct FOrder { TWeakObjectPtr<UObject> AfterOwner; FTickFunction* After; TWeakObjectPtr<UObject> FirstOwner; FTickFunction* First; };
	TArray<FOrder> Orders;
	TSharedPtr<FJsonObject> RoleClasses;
	FDelegateHandle TickStartHandle, TickHandle, SpawnHandle, CueHandle, ProbeHandle;

	static UCombatRecorder* Current;
};
