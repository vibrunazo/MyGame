// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
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
 * A Hitbox Actor. Can have multiple shape components (Box Component or Sphere Component).
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
	void AddComponentsFromContainer(class UHitboxesContainer* Container);
	void AddComponentsFromSettings(struct FHitboxSettings Settings); 
	void AddOneComponentFromBone(FHitboxSettings Settings, FName Bone);
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
	class UHitboxesContainer* Hitboxes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox")
	TArray<UPrimitiveComponent*> HitComponents;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "How many times I can hit the same Actor?"))
	uint8 NumHits = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "Cooldown between hits allowed against the same Actor."))
	float HitCooldown = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ToolTip = "What type of attack this is. Used to determine if it can hit the target. One enemey might be invunerable to weak attacks."))
	EHitboxChannel HitboxChannel = EHitboxChannel::Weak;

	// Which Actors have I already hit and how many times have I hit each one?
	TMap<AActor*, FEnemyHitState> ActorsHit;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void ApplyAllEffects(class IGetHit* Target);
	void ApplyOneEffect(FGameplayEffectSpecHandle Effect, class IGetHit* Target);
	class UMyGameplayAbility* OwningAbility;

};
