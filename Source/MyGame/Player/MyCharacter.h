// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ReportDeath.h"
#include "../Abilities/MyAttributeSet.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "../Abilities/IGetHit.h"
#include "../Abilities/ICastProjectile.h"

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "MyCharacter.generated.h"

//DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FHealthUpdateSignature, AMyCharacter, OnUpdatedHealth, float, NewHealth );

//DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FDieSignature, AMyCharacter, OnDieDelegate, AMyCharacter*, WhoDied);
//DECLARE_MULTICAST_DELEGATE_OneParam(FDieSignature, AMyCharacter*);


UCLASS(config=Game)
class AMyCharacter : public ACharacter, public IAbilitySystemInterface, public IGetHit, public ICastProjectile, public IReportDeath
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AMyCharacter();

	void Tick(float DeltaSeconds) override;
	void Jump() override;
	void FellOutOfWorld(const UDamageType& dmgType) override;
	void OnConstruction(const FTransform& Transform) override;

	virtual void SetDefaultProperties();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void GiveAbility(TSubclassOf<class UGameplayAbility> Ability);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetAbilityKeyDown(uint8 Index, bool IsKeyDown);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool GetAbilityKeyDown(uint8 Index);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void ActivateAbilityByInput(uint8 Index);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void ActivateAbilityByEvent(FString EventName);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void UpdateHealthBar();
	/*UPROPERTY(BlueprintAssignable, Category="Abilities")
	FHealthUpdateSignature OnUpdatedHealth;*/
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void OnPawnSeen(APawn* SeenPawn);
	TSubclassOf<class UCameraShake> GetCamShake();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool HasControl();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	class UMyGameInstance* GetMyGameInstance();
	bool HasStunImmune();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetBodyColor(FLinearColor NewColor);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void ResetBodyColor();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetGlow(float NewGlow);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void ResetGlow();
	UFUNCTION()
	void SetAggroTarget(APawn* NewTarget);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetIsInCombat(bool NewState);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool IsInCombat();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetRunning(bool NewState);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool IsRunning();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetWalking(bool NewState);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool IsWalking();

	//UPROPERTY(BlueprintAssignable, Category="Abilities")
	FDieSignature& GetReportDeathDelegate() override;
	FDieSignature OnDieDelegate;

	FActiveGameplayEffectHandle* OnGetHitByEffect(FGameplayEffectSpecHandle NewEffect, AActor* SourceActor) override;
	UFUNCTION(BlueprintImplementableEvent, Category = Abilities)
	void OnDamagedBP(AActor* SourceActor);
	void OnDamaged(AActor* SourceActor) override;
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void OnDie() override;
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool IsAlive() override;
	void OnHitPause(float Duration) override;
	uint8 GetTeam() override;
	UMyAttributeSet* GetAttributes() override;
	FTransform GetProjectileSpawn() override;
	bool IsValidLowLevel() override;
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void AddItemToInventory(class UItemDataAsset* NewItem) override;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Abilities)
	AActor* GetTargetEnemy();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetTargetEnemy(AActor* NewTarget);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void AddToAggroList(AActor* NewActor);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void RemoveFromAggroList(AActor* NewActor);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void ClearAggroList();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool IsInAggroList(AActor* NewActor);

	void ApplyAllItemEffects();
	void ApplyKnockBack(AActor* SourceActor, float Power);
	void ApplyLaunchBack(AActor* SourceActor, FVector Power);
	void OnDelayedLaunch();
	void OnDelayedLaunch2();
	void OnHitPauseEnd();
	void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode) override;
	void OnSpeedChange(const FOnAttributeChangeData& Data);
	UFUNCTION()
	void PawnBlockTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void RemoveOutline();

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	class UAbilitySystemComponent* AbilitySystem;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	class ULootComponent* LootComponent;
	// UPROPERTY(BlueprintReadOnly, Category = Abilities)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<struct FAbilityStruct> Abilities;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	// TArray<FLootDrop> LootTable;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TSubclassOf<class UCameraShake> CamShakeClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	class ARoomCameraPawn* CamPawnRef = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	float MaxHealth = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	float Attack = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	float Defense = 1.0f;
	float BaseSpeed = 350.0f;
	TArray<bool> IsAbilityKeyDown = { false, false, false, false, false, false, false, false};
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	// TSubclassOf<class UUserWidget> HealthBarWidget;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	class UWidgetComponent* HealthBarComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	class UPawnSensingComponent* PawnSenseComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	class UArrowComponent* SpawnArrow;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	class UBoxComponent* TargetDetection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	uint8 Team = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	bool StunImmune = false;
	uint8 HitStunCount = 0;
	float LastHitstunTime = -990.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	uint8 MaxStuns = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	float StunImmuneCooldown = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	float DoubleTapDelay = 0.1f;
	// How long do I need to press forward for, to start running
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	float TimeRequiredToRun = 1.1f;
	float TimeHoldingRun = 0.f;
	bool bHasControl = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	class UMaterialInstanceDynamic* DynaMat;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	FLinearColor BodyColor = FLinearColor(0.5f, 0.5f, 0.5f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UGameplayEffect> InCombatBuff;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UGameplayEffect> RunBuff;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UGameplayEffect> WalkBuff;
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	TArray<class UItemDataAsset*>* Inventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities")
	class USoundBase* AggroSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities")
	class USoundBase* DeathSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement")
		FRotator WalkRotationRate = FRotator(0.f, 600.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement")
		FRotator RunRotationRate = FRotator(0.f, 200.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement")
		float WalkAccel = 9000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement")
		float RunAccel = 3000.f;
	

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;
private:
	
	void OnDelayedDeath();
	void StartBackslide(FVector Dir);
	void OnBackslide();
	FVector KnockBackVector;
	void CheckWalls();
	void IncrementHitStunCount();
	void CalculateDash(float DeltaSeconds);
	void TryRun(float DeltaSeconds);
	float GetInputAngle();
	void DropItems();
	
	UAnimMontage *GetHitMontage;
	float ForwardAxis = 0.0f;
	float RightAxis = 0.0f;
	float LastInputApexTime = -990.0f;
	float LastInputZeroTime = -990.0f;
	FVector LastInputVector = FVector(0.0f, 0.0f, 0.0f);
	FVector LastLaunchBack = FVector(0.0f, 0.0f, 0.0f);
	class UMyGameInstance* MyGIRef;
	FVector LastGroundLocation = FVector();
	UPROPERTY()
	AActor* TargetEnemy = nullptr;
	UPROPERTY()
	TArray<AActor*> AggroList = TArray<AActor*>();

	// float LastInputAngle = 0.0f;

protected:
	void BeginPlay() override;
	void MoveRight(float Value);
	void MoveForward(float Value);
	void TurnAtRate(float Rate);


	//void SetOutline();
	//FTimerHandle OutlineTimer;
	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);
	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);
	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	//UPROPERTY()
	UPROPERTY(Category=Attributes, VisibleAnywhere, BlueprintReadOnly)
	UMyAttributeSet* AttributeSetBase;

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	//FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	//FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

