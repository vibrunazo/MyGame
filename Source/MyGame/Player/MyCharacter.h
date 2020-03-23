// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "../MyBlueprintFunctionLibrary.h"
#include "../Abilities/GetHit.h"
#include "GameplayEffectTypes.h"
#include "MyCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FHealthUpdateSignature, AMyCharacter, OnUpdatedHealth, float, NewHealth );

UCLASS(config=Game)
class AMyCharacter : public ACharacter, public IAbilitySystemInterface, public IGetHit
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

	UFUNCTION(BlueprintCallable, Category = Abilities)
	void GiveAbility(TSubclassOf<class UGameplayAbility> Ability);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void SetAbilityKeyDown(uint8 Index, bool IsKeyDown);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	bool GetAbilityKeyDown(uint8 Index);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void ActivateAbilityByInput(uint8 Index);
	UFUNCTION(BlueprintCallable, Category = Abilities)
	void UpdateHealthBar();
	UPROPERTY(BlueprintAssignable, Category="Abilities")
	FHealthUpdateSignature OnUpdatedHealth;

	void OnGetHitByEffect(FGameplayEffectSpecHandle NewEffect) override;
	void OnDamaged(AActor* SourceActor) override;
	void OnDie() override;
	bool IsAlive() override;
	void OnHitPause(float Duration) override;

	void OnHitPauseEnd();

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	class UAbilitySystemComponent* AbilitySystem;
	UPROPERTY(BlueprintReadOnly, Category = Abilities)
	class UMyAttributeSet* AttributeSetBase;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<struct FAbilityStruct> Abilities;
	TArray<bool> IsAbilityKeyDown = {false, false, false, false};
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	// TSubclassOf<class UUserWidget> HealthBarWidget;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities)
	class UWidgetComponent* HealthBarComp;


	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;
private:
	UFUNCTION()
	void OnDelayedDeath();
	UAnimMontage *GetHitMontage;

protected:

	void BeginPlay() override;
	void MoveRight(float Value);
	void MoveForward(float Value);
	void TurnAtRate(float Rate);
	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);
	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);
	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

