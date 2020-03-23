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

	void AddComponentsToBones(TArray<FName> Bones);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class USceneComponent* MyRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Hitbox")
	class USphereComponent* MySphere;
	TArray<FGameplayEffectSpecHandle> EffectsToApply;
	TArray<AActor*> ActorsHit;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void ApplyAllEffects(class IGetHit* Target);
	void ApplyOneEffect(FGameplayEffectSpecHandle Effect, class IGetHit* Target);

};
