// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ReportDeath.h"
#include "../Abilities/MyAttributeSet.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "../Abilities/IGetHit.h"
#include "AssCombatant.h"
#include "AssProjectileSpawner.h"

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "MyCharacter.generated.h"


//DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FHealthUpdateSignature, AMyCharacter, OnUpdatedHealth, float, NewHealth );

//DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FDieSignature, AMyCharacter, OnDieDelegate, AMyCharacter*, WhoDied);
//DECLARE_MULTICAST_DELEGATE_OneParam(FDieSignature, AMyCharacter*);




UCLASS(config=Game)
class AMyCharacter : public ACharacter, public IAbilitySystemInterface, public IAssCombatant, public IGetHit, public IAssProjectileSpawner, public IReportDeath
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
	void PossessedBy(AController* NewController) override;

	virtual void SetDefaultProperties();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void GiveAbility(TSubclassOf<class UGameplayAbility> Ability);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void LearnAbility(struct FAbilityStruct Ability);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void LearnAbilities(TArray<struct FAbilityStruct> NewAbilities);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void FindAndRemoveOverlappingAbilities(struct FAbilityStruct Ability);
	// Air moves carry activate.flyingattack (and are blocked on the ground); everything else is a ground move
	static bool IsAirAbility(TSubclassOf<class UGameplayAbility> AbilityClass);
#if WITH_EDITOR
	// PIE test hook: sets an attribute's base value through the ability system (modifiers and damage see it)
	UFUNCTION(BlueprintCallable, Category = "Testing")
	void SetAttributeBaseForTest(FGameplayAttribute Attribute, float Value);
	// PIE test hooks: the character's gameplay tags, and the abilities active right now (class names)
	UFUNCTION(BlueprintCallable, Category = "Testing")
	TArray<FString> GetTagsForTest() const;
	UFUNCTION(BlueprintCallable, Category = "Testing")
	TArray<FString> GetActiveAbilitiesForTest() const;
#endif
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetAbilityKeyDown(uint8 Index, bool IsKeyDown);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool GetAbilityKeyDown(uint8 Index);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void ActivateAbilityByInput(uint8 Index);
	// Movement input for this frame from the controller's move action: X = forward (world +X), Y = right (world +Y)
	void SetMoveInput(FVector2D Value);
	// Dashes towards Direction (world X/Y; zero keeps the current facing) by sending event.dash to the ability system
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void Dash(FVector2D Direction);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void UpdateHealthBar();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void AddDurationToHealthBar(float Duration, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle);
	/*UPROPERTY(BlueprintAssignable, Category="Abilities")
	FHealthUpdateSignature OnUpdatedHealth;*/
	// The AI controller's perception saw SeenPawn (players only)
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void OnPawnSeen(APawn* SeenPawn);
	TSubclassOf<class UCameraShakeBase> GetCamShake();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool HasControl() override;
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool HasMoveControl() override;
	void PlayHitCameraShake(float Strength) override;
	void PlayHitStunMontage() override;
	UFUNCTION(BlueprintCallable, Category = Abilities)
	class UMyGameInstance* GetMyGameInstance();
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
	void EnterCombat() override { SetIsInCombat(true); }
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
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool IsProjectileImmune() override;

	//UPROPERTY(BlueprintAssignable, Category="Abilities")
	FDieSignature& GetReportDeathDelegate() override;
	FDieSignature OnDieDelegate;

	FActiveGameplayEffectHandle OnGetHitByEffect(FGameplayEffectSpecHandle NewEffect) override;
	UFUNCTION(BlueprintImplementableEvent, Category = Abilities)
	void OnDamagedBP(AActor* SourceActor);
	void OnDamaged(AActor* SourceActor, float Damage, FGameplayEffectSpec Effect) override;
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void OnDie() override;
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool IsAlive() override;
	uint8 GetTeam() override;
	UMyAttributeSet* GetAttributes() override;
	FTransform GetProjectileSpawn() override;
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void AddItemToInventory(class UItemDataAsset* NewItem) override;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Abilities)
	AActor* GetTargetEnemy() override;
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetTargetEnemy(AActor* NewTarget) override;
	UPrimitiveComponent* GetTargetDetection() override;
	// PunchSlash and KickSlash
	UNiagaraSystem* GetSlashEffect(EAssSlashKind Kind) override;
	// the punches' sounds are PunchWeakWoosh and PunchStrongWoosh; the kicks play their notify's own
	bool GetStrikeSound(EAssSoundKind Kind, USoundBase*& OutSound) override;
	// GA_Summon's graph: the spawner in this tile's box farthest from the target spawns; none with no target or spawner
	bool Summon(const struct FAssSummon& Summon) override;
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void AddToAggroList(AActor* NewActor);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void RemoveFromAggroList(AActor* NewActor);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void ClearAggroList();
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool IsInAggroList(AActor* NewActor);

	void ApplyAllItemEffects();
	void ApplyOneItemEffect(class UItemDataAsset* NewItem);
	void OnDelayedLaunch2();
	void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode) override;
	void OnSpeedChange(const FOnAttributeChangeData& Data);
	void OnRotSpeedChange(const FOnAttributeChangeData& Data);
	void UpdateRotationRate();
	void OnEffectApplied(UAbilitySystemComponent* SourceComp, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffect);
	void RemoveOutline();

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	class UAssAbilitySystemComponent* AbilitySystem;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	class ULootComponent* LootComponent;
	// The combat mechanics (UAssCombatComponent; COMBAT.md)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UAssCombatComponent> Combat;
	// UPROPERTY(BlueprintReadOnly, Category = Abilities)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<struct FAbilityStruct> Abilities;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	// TArray<FLootDrop> LootTable;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TSubclassOf<class UCameraShakeBase> CamShakeClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	class ARoomCamera* CamPawnRef = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities, meta = (tooltip = "if < 0, starts with MaxHealth, else starts with InitialHealth every level"))
	float InitialHealth = -1.f; 
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
	class UArrowComponent* SpawnArrow;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	class UBoxComponent* TargetDetection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	uint8 Team = 0;
	// The dash's double tap is IA_Dash's Directional Double Tap trigger; these two only drive running:
	// holding past DoubleTapAxisDepth runs, and a quick tap in another direction within DoubleTapDelay stops it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities, meta = (ToolTip = "Seconds between taps for a quick turn to stop running."))
	float DoubleTapDelay = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities, meta = (ToolTip = "How far the stick must go (0..1) to count as held or tapped."))
	float DoubleTapAxisDepth = 0.6f;
	// How long do I need to press forward for, to start running
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	float TimeRequiredToRun = 1.1f;
	float TimeHoldingRun = 0.f;
	// one per material slot of the mesh: body colour and glow are set on all of them (the UE5 mannequin's body has two)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	TArray<TObjectPtr<class UMaterialInstanceDynamic>> BodyMaterials;
	void SetBodyMaterialsVector(FName Parameter, const FLinearColor& Value);
	void SetBodyMaterialsScalar(FName Parameter, float Value);
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NiagaraSystem, meta = (ToolTip = "The niagara system to spawn for the Punch Notifiy states"))
	class UNiagaraSystem* PunchSlash;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NiagaraSystem, meta = (ToolTip = "The niagara system to spawn for the Kick Notifiy states"))
	class UNiagaraSystem* KickSlash;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Woosh Sound", meta = (ToolTip = "The Woosh sound to play on Weak Punches"))
	USoundBase* PunchWeakWoosh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Woosh Sound", meta = (ToolTip = "The Woosh sound to play on Strong Punches"))
	USoundBase* PunchStrongWoosh;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;
private:
	
	void OnDelayedDeath();
	void CheckWalls();
	void UpdateRun(float DeltaSeconds);
	void TryRun(float DeltaSeconds);
	float GetInputAngle();
	void DropItems();
	
	UPROPERTY()
	TObjectPtr<UAnimMontage> GetHitMontage;
	float ForwardAxis = 0.0f;
	float RightAxis = 0.0f;
	float LastInputApexTime = -990.0f;
	float LastInputZeroTime = -990.0f;
	FVector LastInputVector = FVector(0.0f, 0.0f, 0.0f);
	UPROPERTY()
	TObjectPtr<class UMyGameInstance> MyGIRef;
	FVector LastGroundLocation = FVector();
	UPROPERTY()
	AActor* TargetEnemy = nullptr;
	UPROPERTY()
	TArray<AActor*> AggroList = TArray<AActor*>();

	// float LastInputAngle = 0.0f;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

