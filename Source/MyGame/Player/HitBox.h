// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "HitBox.generated.h"

UCLASS()
class MYGAME_API AHitBox : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHitBox();
	virtual void Tick(float DeltaTime) override;
	UFUNCTION()
	void OnHitboxBeginOverlap(AActor* OverlappingActor, AActor* OtherActor);

	//void AddComponentsToBones(TArray<FName> Bones);
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
	TArray<AActor*> ActorsHit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class UHitboxesContainer* Hitboxes;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void ApplyAllEffects(class IGetHit* Target);
	void ApplyOneEffect(FGameplayEffectSpecHandle Effect, class IGetHit* Target);

};
