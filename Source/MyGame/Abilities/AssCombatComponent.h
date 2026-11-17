#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "../Player/HitBox.h"
#include "AssCombatComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class ACharacter;
enum class EHitReaction : uint8;
struct FGameplayEffectSpec;

/**
 * A character's combat mechanics (Ass Plan.md Step 1; COMBAT.md has their clocks and order): knockback and launch,
 * the hit stun count and stun immunity, hit pause, the root-motion speed curves, body-block switching, the airborne
 * tag and the ground and air ability blocks, the control checks, and the open melee hit windows. Its owner is an
 * ACharacter implementing IAssCombatant, and calls the hooks below at fixed points of its own (BeginPlay, movement
 * mode, death) so the frame's order stays as COMBAT.md describes.
 */
UCLASS(ClassGroup = Combat, meta = (BlueprintSpawnableComponent))
class MYGAME_API UAssCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAssCombatComponent();

	// Landed stuns before stun immunity starts; 0 never
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	uint8 MaxStuns = 0;
	// How long stun immunity lasts, in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	float StunImmuneCooldown = 5.0f;
	// The hitbox this character's attacks spawn (its look and sound on hit)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat)
	TSubclassOf<AHitBox> HitBoxClass;
	// Applied for StunImmuneCooldown (data.stunimmune) at the MaxStuns-th landed stun
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat)
	TSubclassOf<UGameplayEffect> StunImmuneEffect;
	// Applied for 0.5 s (data.nocontrol) by a launch
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat)
	TSubclassOf<UGameplayEffect> NoControlEffect;

	// The actor's combat component, or nullptr
	static UAssCombatComponent* Find(const AActor* Actor);

	// The owner's hooks, at its own points: the start of its BeginPlay, after its ability system knows its actor, its
	// movement mode changing, its death
	void BeginCombat();
	void BindAbilitySystem(UAbilitySystemComponent* InAbilitySystem);
	void OnMovementModeChanged();
	// Off when the owner dies or its level is won: it takes no input and HasControl is false
	void SetControlEnabled(bool bEnabled) { bControlEnabled = bEnabled; }
	bool IsControlEnabled() const { return bControlEnabled; }

	// An applied effect's UHitReactionEffectComponent asks for a reaction; the attacker is the spec's effect causer
	void OnHitReaction(EHitReaction Reaction, const FGameplayEffectSpec& Spec);
	void ApplyKnockBack(AActor* SourceActor, float Power);
	void ApplyLaunchBack(AActor* SourceActor, FVector Power);
	// Test only: the landed stuns go back to none (they never decay); returns the count before
	int32 ResetHitStunCount();

	// Freezes the owner (almost) for Duration of world time, from the moment its hit connected
	void OnHitPause(float Duration);
	// A hit of the owner's connected at Time (world time; it can fall between frames)
	void NoteHitConnect(double Time) { LastHitConnectTime = Time; LastHitConnectFrame = GFrameCounter; }

	// No hit stun, no lost control, and not disabled (death)
	bool HasControl() const;
	// As HasControl, and not rooted (Ass.Status.NoMove)
	bool HasMoveControl() const;
	bool HasStunImmune() const;

	// Where the owner was when the frame started: melee hit windows place their samples from there to now
	const FHitboxPose& GetFrameStartPose() const { return FrameStartPose; }
	// Advances this melee hit window after every animation update, until it's destroyed
	void TrackHitWindow(AHitBox* Hitbox) { HitWindows.AddUnique(Hitbox); }

	// The only place that sets the capsule's response to other pawns: ignored while dead, airborne or while an effect
	// grants Ass.Status.NoPawnBlock (moves that pass through enemies); blocked otherwise
	UFUNCTION()
	void RefreshPawnCollision();

protected:
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	ACharacter* GetCharacter() const;
	UAbilitySystemComponent* GetAbilitySystem() const;
	bool IsOwnerAlive() const;

	void IncrementHitStunCount();
	void StartBackslide(FVector Dir);
	void OnBackslide();
	void OnDelayedLaunch();
	UFUNCTION()
	void PawnBlockTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	const ACharacter* FindOverlappingCharacter() const;
	void StepOutOf(const ACharacter* Other);

	// Every frame, before anything ticks: records where the frame starts, and the hit pause's share of the frame
	// sets the owner's time dilation
	void OnFrameStart(UWorld* World, ELevelTick TickType, float DeltaSeconds);
	void SetHitPauseFX(bool bPaused);
	// A montage's RootSpeedX curve (m/s, times the ability's RootMotionScale.X) moves the character forward, at the
	// speed it had at 60 fps (after a frame of braking), averaged over the montage time this frame plays: a root
	// motion source for exactly this frame, so the move covers the same ground at any frame rate
	void ApplyRootSpeedCurve(float DeltaSeconds);
	// after each animation update: open hit windows test up to the montage's new position
	void OnPoseFinalized();

	bool bControlEnabled = true;
	uint8 HitStunCount = 0;
	FVector KnockBackVector = FVector::ZeroVector;
	FVector LastLaunchBack = FVector::ZeroVector;
	FTimerHandle PawnBlockRetryTimer;
	float PawnBlockWaitStart = -1.f;

	// the montage whose RootSpeedX has moved the owner up to RootSpeedCovered (its time), and the last frame's
	// movement time
	TWeakObjectPtr<const class UAnimMontage> RootSpeedMontage;
	float RootSpeedCovered = 0.f;
	float LastMoveTime = 0.f;
	// how fast a hit-paused character runs
	static constexpr float HitPauseRate = 0.01f;
	// the pause, in world time
	double HitPauseStart = 0.0;
	double HitPauseEnd = 0.0;
	// world time the owner ran beyond the pause (a frame runs at full speed past the hit that starts one)
	double HitPauseDebt = 0.0;
	bool bHitPaused = false;
	// when the owner's last hit connected (a hit window's sample time), and in which frame
	double LastHitConnectTime = 0.0;
	uint64 LastHitConnectFrame = 0;

	FHitboxPose FrameStartPose;
	TArray<TWeakObjectPtr<AHitBox>> HitWindows;
	FDelegateHandle PoseFinalizedHandle;
	FDelegateHandle PreActorTickHandle;
};
