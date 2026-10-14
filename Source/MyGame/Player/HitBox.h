// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "HitboxSettings.h"
#include "HitBox.generated.h"


/**
* The type of attack damage this Hitbox does. Used to determine if it can hit the target. One enemy might be immune to weak attacks but vulnerable to casts, or vice versa.
*/
UENUM(BlueprintType)
enum class EHitboxChannel : uint8
{
	Weak = 0,
	Strong = 1,
	Projectile = 2,
	Jump = 3,
};

/**
* Holds the current state for each enemy hit by a Hitbox Actor. Stored in a map in the Hitbox Actor.
*/
USTRUCT(BlueprintType)
struct FEnemyHitState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "How many times did I hit this enemy already? So the Hitbox can know if I already hit it the max number of times and cannot hit it again."))
		uint8 NumHits = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "When was the last time this enemy was hit? So the Hitbox can know if the cooldown is up and it can hit it again."))
		float LastHitTime = 0.0f;
};

/**
 * Where a character was when a frame started (= where the last one ended). Hit windows place their samples between
 * this and where the frame ends, by the sample's montage time.
 */
struct FHitboxPose
{
	TWeakObjectPtr<const class UAnimMontage> Montage;
	float MontagePosition = 0.f;
	double Time = 0.0;
	FTransform MeshToWorld;
	FTransform ActorToWorld;

	void Capture(const class ACharacter* Char, double AtTime);
};

/**
 * A Hitbox Actor. Two ways to hit:
 * - a melee window from a montage's ANS_Hitbox (BeginWindow): its shapes are tested at fixed animation-time steps
 *   through the window, posed by the montage's animation at each step and placed where the character (and each
 *   target) was at that moment, so a move hits the same at any frame rate;
 * - shape components and their overlap events (SetupHitboxes): projectiles and anything not on a montage.
 * Either way a target is hit when it enters a shape, up to NumHits times, HitCooldown apart.
 */
UCLASS()
class MYGAME_API AHitBox : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHitBox();
	virtual void Tick(float DeltaTime) override;

	//void AddComponentsToBones(TArray<FName> Bones);
	void SetOwningAbility(class UMyGameplayAbility* NewOwner);
	// Creates the overlap shapes (one per bone per setting) and the per-target hit limits, from an ANS_Hitbox
	void SetupHitboxes(const TArray<struct FHitboxSettings>& Settings, uint8 InNumHits, float InHitCooldown);
	void AddComponentsFromSettings(struct FHitboxSettings Settings); 
	void AddOneComponentFromBone(FHitboxSettings Settings, FName Bone);
	// Opens a melee window: Settings' shapes on the character's bones, tested from Start to End of the montage's time
	void BeginWindow(class AMyCharacter* Char, const class UAnimMontage* Montage, float Start, float End, const TArray<struct FHitboxSettings>& Settings, uint8 InNumHits, float InHitCooldown);
	// Tests the window up to the montage's current position (every animation update)
	void AdvanceWindow();
	// Tests what's left of the window (it's closing)
	void FinishWindow();
	class USphereComponent* AddHitSphere(float SphereRadius);
	class UBoxComponent* AddHitBoxComponent(FVector BoxExtent);
	UFUNCTION()
	void OnOwnerDestroyed(AActor* DestroyedActor);

	/** Called when the collision capsule touches another primitive component */
	UFUNCTION()
	virtual void HitboxTouched(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class USceneComponent* MyRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class USphereComponent* MySphere;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox")
	//float SphereRadius = 40.0f;
	TArray<FGameplayEffectSpecHandle> EffectsToApply;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class USoundBase* HitSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class USoundBase* BlockSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class UNiagaraSystem* HitParticles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class UNiagaraSystem* BlockParticles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox")
	TArray<UPrimitiveComponent*> HitComponents;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "How many times I can hit the same Actor?"))
	uint8 NumHits = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "Cooldown between hits allowed against the same Actor."))
	float HitCooldown = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "What type of attack this is. Used to determine if it can hit the target. One enemey might be invunerable to weak attacks."))
	EHitboxChannel HitboxChannel = EHitboxChannel::Weak;

	// Which Actors have I already hit and how many times have I hit each one?
	UPROPERTY()
	TMap<TObjectPtr<AActor>, FEnemyHitState> ActorsHit;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Hits Other if the limits allow it and a line from From reaches it; Time is when the contact happened
	void TryHit(AActor* Other, const FVector& From, double Time);
	void ApplyAllEffects(class IGetHit* Target);
	void ApplyOneEffect(FGameplayEffectSpecHandle Effect, class IGetHit* Target);
	TWeakObjectPtr<class UMyGameplayAbility> OwningAbility;

private:
	// a melee window's shape: where it sits on the mesh, and who it touched at the last sample
	struct FWindowShape
	{
		FHitboxSettings Settings;
		int32 Bone = INDEX_NONE;
		FTransform SocketOffset;
		TSet<TWeakObjectPtr<AActor>> Touching;
		// the bone and its parents up to the root, as skeleton indices (to sample the montage's animation)
		TArray<int32> SkeletonChain;
	};
	// tests the window at its sample times up to Position (the montage's time)
	void SampleWindowUntil(float Position);
	// one sample at montage time At, Alpha of the way from where the frame started to now
	void SampleWindowAt(float At, float Alpha, bool bInterpolate, double Time);
	// Shape's bone in the montage's animation at Position (component space), if the montage plays animation data
	bool AnimBoneTransform(const FWindowShape& Shape, float Position, FTransform& OutComponentSpace) const;

	// the window's samples are this far apart in animation time (a 60 fps frame)
	static constexpr float WindowStep = 1.f / 60.f;
	// characters this much further than a shape's reach are candidates too (they're tested where they were at the sample)
	static constexpr float CandidateMargin = 200.f;
	TArray<FWindowShape> WindowShapes;
	TWeakObjectPtr<class AMyCharacter> WindowChar;
	TWeakObjectPtr<const class UAnimMontage> WindowMontage;
	float WindowEnd = 0.f;
	float NextSample = 0.f;
	bool bWindowDone = true;
};
