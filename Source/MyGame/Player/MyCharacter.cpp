// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MyCharacter.h"
#include "EnemyCharBase.h"
#include "MyPlayerController.h"
#include "../MyGameInstance.h"
#include "../Abilities/LootComponent.h"
#include "../Level/LevelBuilder.h"
#include "../Props/ItemDataAsset.h"
#include "../Props/Pickup.h"
#include "../UI/MyHealthBar.h"

//#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
// #include "../Abilities/MyAttributeSet.h"
#include "MyAnimInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
//#include "Kismet/GameplayStatics.h"
#include "Perception/PawnSensingComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/ArrowComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

//////////////////////////////////////////////////////////////////////////
// AMyGameCharacter

AMyCharacter::AMyCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(36.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 2000.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 400.f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxWalkSpeed = 350.0f;
	GetCharacterMovement()->GroundFriction = 0.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	//CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	//CameraBoom->SetupAttachment(RootComponent);
	//CameraBoom->TargetArmLength = 700.0f; // The camera follows at this distance behind the character	
	//// CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	//CameraBoom->SetWorldRotation(FRotator(-40.0f, 0.0f, 0.0f));
	//CameraBoom->bInheritYaw = false;
	//CameraBoom->bInheritPitch = false;
	//CameraBoom->bInheritRoll = false;
	//CameraBoom->bDoCollisionTest = false;
	//CameraBoom->bEnableCameraLag = true;
	//CameraBoom->CameraLagSpeed = 2.0f;
	//CameraBoom->CameraLagMaxDistance = 200.0f;
	//// Create a follow camera
	//FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	//FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	//FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	HealthBarComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComponent"));
	HealthBarComp->SetupAttachment(RootComponent);

	SpawnArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Arrow"));
	SpawnArrow->SetupAttachment(RootComponent);
	SpawnArrow->SetRelativeLocation(FVector(50.0f, 0.0f, 10.0f));
	SpawnArrow->bTreatAsASprite = true;

	TargetDetection = CreateDefaultSubobject<UBoxComponent>(TEXT("Target Box"));
	TargetDetection->SetupAttachment(RootComponent);
	TargetDetection->SetRelativeLocation(FVector(400.f, 0.f, 0.f));
	TargetDetection->SetBoxExtent(FVector(400.f, 300.f, 200.f));
	TargetDetection->SetGenerateOverlapEvents(false);
	TargetDetection->SetVisibility(false);
	
	PawnSenseComp = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("Pawn Sensing"));
	PawnSenseComp->OnSeePawn.AddDynamic(this, &AMyCharacter::OnPawnSeen);

	// Our ability system component.
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSetBase = CreateDefaultSubobject<UMyAttributeSet>(TEXT("AttributeSetBase"));
	LootComponent = CreateDefaultSubobject<ULootComponent>(TEXT("Loot Component"));

	if (AbilitySystem) AbilitySystem->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(FName("status.nopawnblock")), EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMyCharacter::PawnBlockTagChanged);

	SetDefaultProperties();
}

void AMyCharacter::SetDefaultProperties()
{
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -92.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	// DynaMat = GetMesh()->CreateDynamicMaterialInstance(0);
	// if (DynaMat) DynaMat->SetVectorParameterValue(FName("BodyColor"), BodyColor);

	HealthBarComp->SetRelativeLocation(FVector(0.0f, 0.0f, -140.0f));
	HealthBarComp->SetWidgetSpace(EWidgetSpace::Screen);

	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	if (!Anim)
	{
		static ConstructorHelpers::FClassFinder<UMyAnimInstance> AnimClassFinder(TEXT("/Game/Anims/ABP_CharAnim"));
    	TSubclassOf<UMyAnimInstance> AnimClass = AnimClassFinder.Class;
		GetMesh()->SetAnimInstanceClass(AnimClass);
	}
	UMyAnimInstance* MyAnim = Cast<UMyAnimInstance>(Anim);
	if (MyAnim) 
	{
		GetHitMontage = MyAnim->GetHitMontage;
	} 
	static ConstructorHelpers::FClassFinder<AController> AIClassFinder(TEXT("/Game/Blueprints/AI/BP_AIController"));
    TSubclassOf<AController> AIClass = AIClassFinder.Class;
	AIControllerClass = AIClass;
	Team = 0;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMyCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	// PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	/*PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMyCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);*/

	PlayerInputComponent->BindAxis("MoveForward", this, &AMyCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	// PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	// PlayerInputComponent->BindAxis("TurnRate", this, &AMyCharacter::TurnAtRate);
	// PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	// PlayerInputComponent->BindAxis("LookUpRate", this, &AMyCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMyCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMyCharacter::TouchStopped);

}

void AMyCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AMyCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AMyCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMyCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMyCharacter::MoveForward(float Value)
{
	ForwardAxis = Value;
	if ((Controller != NULL) && (Value != 0.0f) && HasControl())
	{
		// find out which way is forward
		// const FRotator Rotation = Controller->GetControlRotation();
		// const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		// const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector Direction = FVector(1.0f, 0.0f, 0.0f);
		AddMovementInput(Direction, Value);
		FVector CurVector = FVector(ForwardAxis, RightAxis, 0.0f);
		if (GetController()) GetController()->SetControlRotation(CurVector.Rotation());
	}
}

void AMyCharacter::MoveRight(float Value)
{
	RightAxis = Value;
	if ( (Controller != NULL) && (Value != 0.0f)  && HasControl())
	{
		// find out which way is right
		// const FRotator Rotation = Controller->GetControlRotation();
		// const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		// const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		FVector Direction = FVector(0.0f, 1.0f, 0.0f);
		// add movement in that direction
		AddMovementInput(Direction, Value);
		FVector CurVector = FVector(ForwardAxis, RightAxis, 0.0f);
		if (GetController()) GetController()->SetControlRotation(CurVector.Rotation());
	}
}

UAbilitySystemComponent* AMyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
};

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if(!AbilitySystem) return;
	for (auto &&Ability : Abilities)
	{
		GiveAbility(Ability.AbilityClass);
	}
	if (!ensure(AttributeSetBase != nullptr)) return;
	if (!ensure(GetCharacterMovement() != nullptr)) return;
	AttributeSetBase->SetMaxHealth(MaxHealth);
	AttributeSetBase->SetAttack(Attack);
	AttributeSetBase->SetDefense(Defense);
	BaseSpeed = GetCharacterMovement()->MaxWalkSpeed;
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetSpeedAttribute()).AddUObject(this, &AMyCharacter::OnSpeedChange);
	AbilitySystem->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &AMyCharacter::OnEffectApplied);
	// if (IsPlayerControlled()) 
	// {
	// 	Team = 1;
	// }
		// else Team = 0;
	//DynaMat = GetMesh()->CreateDynamicMaterialInstance(0);
	//ResetBodyColor();
	if (IsPlayerControlled() && GetMyGameInstance() && AttributeSetBase)
	{
		if (InitialHealth < 0.f) AttributeSetBase->SetHealth(GetMyGameInstance()->Health);
		else AttributeSetBase->SetHealth(InitialHealth);
		AttributeSetBase->SetMana(GetMyGameInstance()->Mana);
		GetMyGameInstance()->SetCharRef(this);
		Inventory = &GetMyGameInstance()->Inventory;
		ApplyAllItemEffects();
	}
	else
	{
		if (InitialHealth < 0.f) AttributeSetBase->SetHealth(MaxHealth);
		else AttributeSetBase->SetHealth(InitialHealth);
	}
	UpdateHealthBar();
	SetIsInCombat(true);
}


void AMyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (InputEnabled())
	{
		if (GetAbilityKeyDown(0)) ActivateAbilityByInput(0);
		if (GetAbilityKeyDown(1)) ActivateAbilityByInput(1);
		if (GetAbilityKeyDown(2)) ActivateAbilityByInput(2);
		if (GetAbilityKeyDown(3)) ActivateAbilityByInput(3);
		if (GetAbilityKeyDown(4)) ActivateAbilityByInput(4);
		if (GetAbilityKeyDown(5)) ActivateAbilityByInput(5);
		if (GetAbilityKeyDown(6)) ActivateAbilityByInput(6);
		if (GetAbilityKeyDown(7)) ActivateAbilityByInput(7);
	}
	if (IsPlayerControlled())
	{
		CheckWalls();
		CalculateDash(DeltaSeconds);
	}

}

void AMyCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	DynaMat = GetMesh()->CreateDynamicMaterialInstance(0);
	ResetBodyColor();

}

/// <summary>
/// Calculates if I should dash this tick
/// </summary>
void AMyCharacter::CalculateDash(float DeltaSeconds)
{
	// TODO move to controller
	float length = 0.0f;
	length = FMath::Abs(RightAxis)*FMath::Abs(RightAxis) + FMath::Abs(ForwardAxis)*FMath::Abs(ForwardAxis);
	length = FMath::Sqrt(length);
	if (length >= DoubleTapAxisDepth)
	{
		TryRun(DeltaSeconds);
		// float CurAngle = GetInputAngle();
		// float Delta = FMath::FindDeltaAngleDegrees(CurAngle, 45.0f);
		FVector CurVector = FVector(ForwardAxis, RightAxis, 0.0f);
		// LastInputZeroTime > LastInputApexTime means this is a tap and not a hold
		// So if this a tap AND the time before last tap was short
		if (GetWorld()->GetTimeSeconds() - LastInputApexTime < DoubleTapDelay && LastInputZeroTime > LastInputApexTime)
		{
			float cos = CurVector.CosineAngle2D(LastInputVector);
			float acos = UKismetMathLibrary::DegAcos(cos);
			// UE_LOG(LogTemp, Warning, TEXT("Forward: %f, Right: %f, len: %f, angle: %f"), ForwardAxis, RightAxis, length, acos);
			// if the angle between last tap and current tap is less than 20 degrees
			if (acos <= 20)
			{
				if (GetController()) GetController()->SetControlRotation(CurVector.Rotation());
				ActivateAbilityByEvent("dash");
			}
			else SetRunning(false);
		}
		// if this is a tap and not a hold
		if (LastInputZeroTime > LastInputApexTime)
		{
			LastInputVector = CurVector;
			LastInputApexTime = GetWorld()->GetTimeSeconds();
		}
	}
	else
	{
		LastInputZeroTime = GetWorld()->GetTimeSeconds();
		/*SetRunning(false);*/
	}

}

void AMyCharacter::TryRun(float DeltaSeconds)
{
	if (!IsInCombat() && !IsRunning())
	{
		TimeHoldingRun += DeltaSeconds;
		if (TimeHoldingRun >= TimeRequiredToRun)
		{
			TimeHoldingRun = TimeHoldingRun;
			SetRunning(true);
		}
	}
}

float AMyCharacter::GetInputAngle()
{
	// UKismetMathLibrary::DegAtan2
	return UKismetMathLibrary::DegAtan2(ForwardAxis, RightAxis);
	// return FVector(ForwardAxis, RightAxis, 0.0f);
}

void AMyCharacter::Jump()
{
	if (!HasControl()) return;
	SetIsInCombat(true);
	Super::Jump();
}

void AMyCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
	if (IsAlive() && AttributeSetBase) 
	{
		AttributeSetBase->SetHealth(0.f);
		OnDie();
	}
}


void AMyCharacter::GiveAbility(TSubclassOf<class UGameplayAbility> Ability)
{
	if (HasAuthority() && Ability)
	{
		AbilitySystem->GiveAbility(FGameplayAbilitySpec(Ability.GetDefaultObject(), 1, 0));
	}
	AbilitySystem->InitAbilityActorInfo(this, this);
}

bool AMyCharacter::GetAbilityKeyDown(uint8 Index)
{
	if (Index >= IsAbilityKeyDown.Num()) return false;
	return IsAbilityKeyDown[Index];
}

void AMyCharacter::SetAbilityKeyDown(uint8 Index, bool IsKeyDown)
{
	if (Index >= IsAbilityKeyDown.Num()) return;
	IsAbilityKeyDown[Index] = IsKeyDown;
}

void AMyCharacter::ActivateAbilityByInput(uint8 Index)
{
	if (!HasControl() || !AbilitySystem) return;
	for (auto &&Ability : Abilities)
	{
		if (Ability.Input == Index && Ability.EventName == "")
		{
			if ((Ability.CanUseOnAir && GetMovementComponent()->IsFalling())
			|| (Ability.CanUseOnGround && !GetMovementComponent()->IsFalling()))
			{
				AbilitySystem->TryActivateAbilityByClass(Ability.AbilityClass, true);
			}
		}
	}
}

void AMyCharacter::ActivateAbilityByEvent(FString EventName)
{
	if (!HasControl() || !AbilitySystem) return;
	for (auto &&Ability : Abilities)
	{
		if (Ability.EventName == EventName)
		{
			if ((Ability.CanUseOnAir && GetMovementComponent()->IsFalling())
			|| (Ability.CanUseOnGround && !GetMovementComponent()->IsFalling()))
			{
				AbilitySystem->TryActivateAbilityByClass(Ability.AbilityClass, true);
			}
		}
	}
}

void AMyCharacter::UpdateHealthBar()
{
	if (!ensure(HealthBarComp != nullptr)) return;
	// if (!ensure(AttributeSetBase != nullptr)) return;
	if (!AttributeSetBase)
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginPlay: AttributeSet NOT created on %s"), *GetName());
		return;
	}
	if (!AbilitySystem) return;
	UUserWidget* Widget = HealthBarComp->GetUserWidgetObject();
	UMyHealthBar* HealthBar = Cast<UMyHealthBar>(Widget);
	float OldHealth = 0.f;
	float NewHealth = AttributeSetBase->GetHealth();
	if (HealthBar)
	{
		OldHealth = HealthBar->GetHealth();
		HealthBar->SetMaxHealth(AttributeSetBase->GetMaxHealth());
		HealthBar->SetHealth(NewHealth);
	}
	AMyPlayerController* MyCont = Cast<AMyPlayerController>(GetController());
	if (MyCont)
	{
		OldHealth = MyCont->GetHUDHealth();
		MyCont->UpdateHUD(this);
	}
	float OldHealthPct = OldHealth / AttributeSetBase->GetMaxHealth();
	float NewHealthPct = NewHealth / AttributeSetBase->GetMaxHealth();
	// TODO really? that's ridiculous, refactor this crap
	if (NewHealthPct <= 0.75f && OldHealthPct > 0.75f)
	{
		ActivateAbilityByEvent("health75");
		FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("status.health.75"));
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, HealthTag, FGameplayEventData());
	}
	if (NewHealthPct <= 0.7f && OldHealthPct > 0.7f)
	{
		ActivateAbilityByEvent("health70");
		FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("status.health.70"));
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, HealthTag, FGameplayEventData());
	}
	if (NewHealthPct <= 0.5f && OldHealthPct > 0.5f)
	{
		ActivateAbilityByEvent("health50");
		FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("status.health.50"));
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, HealthTag, FGameplayEventData());
	}
	if (NewHealthPct <= 0.3f && OldHealthPct > 0.3f)
	{
		ActivateAbilityByEvent("health30");
		FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("status.health.30"));
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, HealthTag, FGameplayEventData());
	}
	if (NewHealthPct <= 0.25f && OldHealthPct > 0.25f)
	{
		ActivateAbilityByEvent("health25");
		FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("status.health.25"));
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, HealthTag, FGameplayEventData());
	}

}


void AMyCharacter::AddDurationToHealthBar(float Duration, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	UUserWidget* Widget = HealthBarComp->GetUserWidgetObject();
	UMyHealthBar* HealthBar = Cast<UMyHealthBar>(Widget);
	if (HealthBar)
	{
		HealthBar->AddDurationToHealthBar(Duration, EffectSpec, ActiveEffectHandle);
	}
}

void AMyCharacter::OnSpeedChange(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * AttributeSetBase->GetSpeed();
	// UE_LOG(LogTemp, Warning, TEXT("My Speed changed to %f"), GetCharacterMovement()->MaxWalkSpeed);
}

void AMyCharacter::OnEffectApplied(UAbilitySystemComponent* SourceComp, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	auto ActiveEffect = AbilitySystem->GetActiveGameplayEffect(ActiveEffectHandle);
	if (!ActiveEffect) return;
	float Duration = ActiveEffect->GetDuration();
	UE_LOG(LogTemp, Warning, TEXT("On Effect Applied of duration: %f"), Duration);
	if (Duration > 1.f) AddDurationToHealthBar(Duration, EffectSpec, ActiveEffectHandle);

	auto ContextHandle = EffectSpec.GetEffectContext();
	auto Context = ContextHandle.Get();

	EffectSpec.Def->UIData;
}

void AMyCharacter::PawnBlockTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount) GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	else GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
}

FActiveGameplayEffectHandle* AMyCharacter::OnGetHitByEffect(FGameplayEffectSpecHandle NewEffect, AActor* SourceActor)
{
	// UE_LOG(LogTemp, Warning, TEXT("Char getting effected"));
	FGameplayTagContainer EffectTags;
	// NewEffect.Data->GetAllGrantedTags(EffectTags);
	if (!ensure(NewEffect.Data)) return nullptr;
	if (!ensure(AbilitySystem)) return nullptr;
	// if (!NewEffect.Data || !AbilitySystem) { return nullptr; }
	NewEffect.Data->GetAllAssetTags(EffectTags);
	// const FActiveGameplayEffect* AGE = AbilitySystem->GetActiveGameplayEffect(NewEffect);
	// FGameplayTag HitstunTag = FGameplayTag::RequestGameplayTag(TEXT("status.hitstun"));
	// the 'data.noapply' tag let's us know this effect only has side effects, and doesn't modify any attributes
	// so we should NOT call ApplyGameplayEffectSpecToSelf
	FGameplayTag NoApplyTag = FGameplayTag::RequestGameplayTag(TEXT("data.noapply"));
	FGameplayTag HitstunTag = FGameplayTag::RequestGameplayTag(TEXT("data.hitstun"));
	FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("data.knockback"));
	FGameplayTag CamShakeTag = FGameplayTag::RequestGameplayTag(TEXT("data.camshake"));
	//FGameplayTag NoPawnBlockTag = FGameplayTag::RequestGameplayTag(TEXT("data.nopawnblock"));
	FGameplayTag LaunchTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch")); 
	FGameplayTag LaunchXTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.x")); 
	FGameplayTag LaunchYTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.y")); 
	FGameplayTag LaunchZTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.z")); 
	// {{TagName="data.knockback" },500.000000}
	if (EffectTags.HasTag(HitstunTag)) 
	{
		// UE_LOG(LogTemp, Warning, TEXT("Has Hitstun Tag, Count: %d, Immune: %d"), HitStunCount, StunImmune);
		if (!HasStunImmune())
		{
			PlayAnimMontage(GetHitMontage);
			IncrementHitStunCount();
		}
	}
	if (EffectTags.HasTag(KnockbackTag))
	{
		float Knockback = NewEffect.Data.Get()->GetSetByCallerMagnitude(KnockbackTag);
		ApplyKnockBack(SourceActor, Knockback);
	}
	if (EffectTags.HasTag(CamShakeTag))
	{
		float CamShakePower = NewEffect.Data.Get()->GetSetByCallerMagnitude(CamShakeTag);
		if (GetMyGameInstance()) GetMyGameInstance()->DoCamShake(CamShakePower);
	}
	/*if (EffectTags.HasTag(NoPawnBlockTag))
	{
		AbilitySystem->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(FName("status.nopawnblock")), EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMyCharacter::PawnBlockTagChanged);
	}*/
	if (EffectTags.HasTag(LaunchTag)) 
	// if (EffectTags.HasTag(LaunchTag) && !StunImmune) 
	{
		float LaunchX = NewEffect.Data.Get()->GetSetByCallerMagnitude(LaunchXTag);
		float LaunchY = NewEffect.Data.Get()->GetSetByCallerMagnitude(LaunchYTag);
		float LaunchZ = NewEffect.Data.Get()->GetSetByCallerMagnitude(LaunchZTag);
		FVector LaunchVector = FVector(LaunchX, LaunchY, LaunchZ);
		// TMap<FGameplayTag, float> KnockbackMap = NewEffect.Data.Get()->SetByCallerTagMagnitudes;
		// float Knockback = *(KnockbackMap.Find(KnockbackTag));
		// UE_LOG(LogTemp, Warning, TEXT("Has Launchtag, Knockback: %s"), *LaunchVector.ToString());
		ApplyLaunchBack(SourceActor, LaunchVector);
	}
	// the 'data.noapply' tag let's us know this effect only has side effects, and doesn't modify any attributes
	// so we should NOT call ApplyGameplayEffectSpecToSelf
	if (EffectTags.HasTag(NoApplyTag))
	{
		return new FActiveGameplayEffectHandle();
	}
	FActiveGameplayEffectHandle ActiveEffect = AbilitySystem->ApplyGameplayEffectSpecToSelf(*(NewEffect.Data.Get()));
	// FActiveGameplayEffectHandle* ActiveEffectPointer = &ActiveEffect;
	UpdateHealthBar();
	return new FActiveGameplayEffectHandle(ActiveEffect);
}

/* Removes the outline of an enemy character by setting the custom depth stencil back to zero.
Called from a timer set by SetOutline() and by the OnDie() event */
void AMyCharacter::RemoveOutline()
{
	GetMesh()->SetRenderCustomDepth(false);
	GetMesh()->SetCustomDepthStencilValue(0);

	TArray<USceneComponent*> ChildrenComponents;
	GetMesh()->GetChildrenComponents(false, ChildrenComponents);

	for (USceneComponent* Component : ChildrenComponents)
	{
		if (!Component->GetOwner()) continue;
		if (UStaticMeshComponent* ChildMesh = Cast<UStaticMeshComponent>(Component))
		{
			ChildMesh->SetRenderCustomDepth(false);
			ChildMesh->SetCustomDepthStencilValue(0);
		}
	}
}

void AMyCharacter::IncrementHitStunCount()
{
	// if (StunImmune || GetWorld()->GetTimeSeconds() > LastHitstunTime + StunImmuneCooldown) {HitStunCount = 0; StunImmune = false;}
	HitStunCount++;
	// LastHitstunTime = GetWorld()->GetTimeSeconds();
	if (MaxStuns > 0 && HitStunCount >= MaxStuns)
	{
		//UE_LOG(LogTemp, Warning, TEXT("%s is Stunimmune"), *GetName());
		StunImmune = true;
		HitStunCount = 0;
		TSubclassOf<UGameplayEffect> StunImmuneEffect = GetMyGameInstance()->StunImmuneEffectRef;
		const FGameplayEffectSpecHandle Handle = AbilitySystem->MakeOutgoingSpec(StunImmuneEffect, 0.f, AbilitySystem->MakeEffectContext());
		FGameplayTag StunImmuneTag = FGameplayTag::RequestGameplayTag(TEXT("data.stunimmune"));
		Handle.Data.Get()->SetSetByCallerMagnitude(StunImmuneTag, StunImmuneCooldown);
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*(Handle.Data.Get()));
	}
}

/* Returns true if the Character is immune to stun. Checks if the Char has the Gameplay Tag status.stunimmune */
bool AMyCharacter::HasStunImmune()
{
	FGameplayTag ImmuneTag = FGameplayTag::RequestGameplayTag(TEXT("status.stunimmune"));
    if(AbilitySystem->HasMatchingGameplayTag(ImmuneTag))
	{
		return true;
	}
	return false;
}

void AMyCharacter::OnDamaged(AActor* SourceActor, float Damage, FGameplayEffectSpec Effect)
{
	// if (!ensure(GetHitMontage != nullptr)) return;
	SetIsInCombat(true);
	if (!GetHitMontage)
	{
		UAnimInstance* Anim = GetMesh()->GetAnimInstance();
		UMyAnimInstance* MyAnim = Cast<UMyAnimInstance>(Anim);
		if (!ensure(MyAnim != nullptr)) return;
		GetHitMontage = MyAnim->GetHitMontage;
	}
	// UE_LOG(LogTemp, Warning, TEXT("I was damaged"));
	FGameplayTagContainer tags = FGameplayTagContainer();
	Effect.GetAllAssetTags(tags);
	UE_LOG(LogTemp, Warning, TEXT("damage effect, tags: %s"), *tags.ToString()); 
	/*FGameplayTag HitStun = FGameplayTag::RequestGameplayTag(TEXT("data.hitstun"));
	if (!HasStunImmune() && tags.HasTag(HitStun)) PlayAnimMontage(GetHitMontage);*/
	//UGameplayStatics::PlayWorldCameraShake(GetWorld(), GetCamShake(), GetActorLocation(), 0.0f, CamShakeRange);
	
	if (IsPlayerControlled())
	{
		AEnemyCharBase* SourceChar = Cast<AEnemyCharBase>(SourceActor);
		if (SourceChar && !SourceChar->IsPlayerControlled())
		{
			SourceChar->SetOutline();
		}
	}
	UpdateHealthBar();
	OnDamagedBP(SourceActor);
}

void AMyCharacter::StartBackslide(FVector Dir)
{
	for (uint8 i = 0; i < 10; i++)
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, this, &AMyCharacter::OnBackslide, i*0.01f, false);
	}
}

void AMyCharacter::OnBackslide()
{
	AddActorWorldOffset(KnockBackVector*0.01f, true);
}

void AMyCharacter::OnDie()
{
	// UE_LOG(LogTemp, Warning, TEXT("I died"));
	if (IsAlive() && AttributeSetBase)
	{
		AttributeSetBase->SetHealth(0.f);
	}
	UWorld* World = GetWorld();
	// FTimerManager TM = FTimerManager::FTimerManager;
	FTimerHandle Handle;
	FTimerHandle Handle2;
	GetWorldTimerManager().SetTimer(Handle, this, &AMyCharacter::OnDelayedDeath, 5.0f, false);
	GetWorldTimerManager().SetTimer(Handle2, this, &AMyCharacter::OnDelayedLaunch2, .05f, false);
	bHasControl = false;
	DisableInput(nullptr);
	if (HealthBarComp) HealthBarComp->SetVisibility(false);
	AMyPlayerController* MyCont = Cast<AMyPlayerController>(GetController());
	if (MyCont)
	{
		MyCont->OnCharDies(this);
	}
	if (!IsPlayerControlled())
	{
		DetachFromControllerPendingDestroy();
		DropItems();
		RemoveOutline();
	}
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	// GetMesh()->SetPhysicsLinearVelocity(FVector(200.f, 0.f, 5000.f));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	UMyAnimInstance* MyAnim = Cast<UMyAnimInstance>(Anim);

	if (DeathSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, GetActorLocation());

	OnDieDelegate.Broadcast(this);
	// if (MyAnim)
	// {
		// MyAnim->StartRagdoll();
	// }
}

void AMyCharacter::OnDelayedLaunch2()
{
	float rx = FMath::RandRange(-100.f, 100.f);
	float ry = FMath::RandRange(-100.f, 100.f);
	GetMesh()->AddImpulse(FVector(8.0f*rx, 8.0f*ry, 2200.0f + 5.f*rx), NAME_None, true);
	GetMesh()->AddForce(FVector(800.0f*rx, 800.0f*ry, 200000.0f), NAME_None, true);
	// LaunchCharacter(FVector(200.f, 0.f, 5000.f), true, true);
}
void AMyCharacter::OnDelayedDeath()
{
	AMyPlayerController* MyCont = Cast<AMyPlayerController>(GetController());
	if (MyCont)
	{
		MyCont->OnDelayedCharDies(this);
	}
	// GetMesh()->SetSimulatePhysics(false);
	GetMesh()->PutAllRigidBodiesToSleep();
	// GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// UE_LOG(LogTemp, Warning, TEXT("Delayed Death"));
	
	// Destroy();
}

void AMyCharacter::DropItems()
{
	// UE_LOG(LogTemp, Warning, TEXT("dropping items"));
	FVector Loc;
	if (GetMovementComponent() && GetMovementComponent()->IsFalling()) Loc = LastGroundLocation;
	else Loc = GetActorLocation();
    FActorSpawnParameters params;
    params.bNoFail = true;
    params.Instigator = this;
    params.Owner = this;
	if (!LootComponent) return;
	auto LootTable = LootComponent->LootTable;
	auto NewLoot = LootComponent->GetRandomItem();
	if (!NewLoot) return;
	APickup* NewPickup = GetWorld()->SpawnActor<APickup>(NewLoot->PickupActor, Loc, FRotator::ZeroRotator, params);
	NewPickup->SetItemData(NewLoot);
	// for (auto &&Loot : LootTable)
	// {
	// 	if (!Loot.Item) continue;
    // 	APickup* NewPickup = GetWorld()->SpawnActor<APickup>(Loot.Item->PickupActor, Loc, FRotator::ZeroRotator, params);
	// 	NewPickup->SetItemData(Loot.Item);
	// }
	
}

bool AMyCharacter::IsAlive()
{
	if (!ensure(AttributeSetBase != nullptr)) return 0;
	return AttributeSetBase->GetHealth() > 0;
}

bool AMyCharacter::IsValidLowLevel()
{
	return Super::IsValidLowLevel();
}

void AMyCharacter::OnHitPause(float Duration)
{
	CustomTimeDilation = 0.01f;
	// UGameplayStatics::SetGlobalTimeDilation(this, 0.01f);
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AMyCharacter::OnHitPauseEnd, Duration, false);

	if (!GetMesh()) return;
	TArray<USceneComponent*> MeshChildren;
	GetMesh()->GetChildrenComponents(false, MeshChildren);

	for (USceneComponent* Component : MeshChildren)
	{
		//if (UFXSystemComponent* FXSystemComponent = Cast<UFXSystemComponent>(Component))
		if (UNiagaraComponent* FXSystemComponent = Cast<UNiagaraComponent>(Component))
		{
			FXSystemComponent->SetPaused(true);
		}
	}
}

void AMyCharacter::OnHitPauseEnd()
{
	CustomTimeDilation = 1.0f;
	// UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	if (!GetMesh()) return;
	TArray<USceneComponent*> MeshChildren;
	GetMesh()->GetChildrenComponents(false, MeshChildren);

	for (USceneComponent* Component : MeshChildren)
	{
		if (UNiagaraComponent* FXSystemComponent = Cast<UNiagaraComponent>(Component))
		{
			FXSystemComponent->SetPaused(false);
		}
	}
}

UMyGameInstance* AMyCharacter::GetMyGameInstance()
{
	if (MyGIRef) return MyGIRef;
	UGameInstance* GI = GetGameInstance();
	if (!ensure(GI != nullptr)) return nullptr;
	MyGIRef = Cast<UMyGameInstance>(GI);
	return MyGIRef;
}

TSubclassOf<UCameraShake> AMyCharacter::GetCamShake()	
{
	if (!CamShakeClass)
	{
		if (!ensure(GetWorld() != nullptr)) return nullptr;
		UGameInstance* GI = GetGameInstance();
		if (!ensure(GI != nullptr)) return nullptr;
		UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
		if (MyGI) {
			CamShakeClass = MyGI->CamShakeClass;
		}
	}
	return CamShakeClass;
}

void AMyCharacter::OnPawnSeen(APawn* SeenPawn)
{
	//UE_LOG(LogTemp, Warning, TEXT("%s Seen Pawn"), *GetName());
	SetAggroTarget(SeenPawn);
}

void AMyCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PrevCustomMode);
	FGameplayTag FlyingTag = FGameplayTag::RequestGameplayTag(TEXT("activate.flyingattack"));
	FGameplayTagContainer FlyingTagContainer = FGameplayTagContainer(FlyingTag);
	FGameplayTag GroundTag = FGameplayTag::RequestGameplayTag(TEXT("activate.groundattack"));
	FGameplayTagContainer GroundTagContainer = FGameplayTagContainer(GroundTag);
	if (!GetMovementComponent()->IsFalling())	// I'm on ground
	{
		AbilitySystem->BlockAbilitiesWithTags(FlyingTagContainer);
		AbilitySystem->UnBlockAbilitiesWithTags(GroundTagContainer);
		AbilitySystem->CancelAbilities(&FlyingTagContainer);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
		GetCapsuleComponent()->SetGenerateOverlapEvents(true);
		GetMesh()->SetGenerateOverlapEvents(false);
	}
	else										// I'm falling
	{
		AbilitySystem->BlockAbilitiesWithTags(GroundTagContainer);
		AbilitySystem->UnBlockAbilitiesWithTags(FlyingTagContainer);
		AbilitySystem->CancelAbilities(&GroundTagContainer);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		GetCapsuleComponent()->SetGenerateOverlapEvents(false);
		GetMesh()->SetGenerateOverlapEvents(true);
		LastGroundLocation = GetActorLocation();
	}
}

uint8 AMyCharacter::GetTeam()
{
	return Team;
}

void AMyCharacter::AddItemToInventory(UItemDataAsset* NewItem)
{
	UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToChar(this, NewItem->EffectsToApply, NewItem);
	if (!NewItem->bIsConsumable)
	{
		if (!Inventory) {
			UE_LOG(LogTemp, Error, TEXT("Failed to add item to inventory. Could not find Inventory on Char."));
			return;
		}
		(*Inventory).Add(NewItem);
	}
}

AActor* AMyCharacter::GetTargetEnemy()
{
	return TargetEnemy;
}

void AMyCharacter::SetTargetEnemy(AActor* NewTarget)
{
	TargetEnemy = NewTarget;
}

void AMyCharacter::AddToAggroList(AActor* NewActor)
{
	if (NewActor)
	{
		AggroList.Add(NewActor);
	}
}

void AMyCharacter::RemoveFromAggroList(AActor* NewActor)
{
	if (!NewActor)
	{
		ClearAggroList();
		return;
	}
	AggroList.Remove(NewActor);
}

void AMyCharacter::ClearAggroList()
{
	AggroList.Empty();
}

bool AMyCharacter::IsInAggroList(AActor* NewActor)
{
	if (AggroList.Num() == 0) return true;
	for (auto&& Enemy : AggroList)
	{
		if (!Cast<AMyCharacter>(Enemy) || !Cast<AMyCharacter>(Enemy)->IsAlive())
		{
			AggroList.Empty();
			return true;
		}
		if (Enemy == NewActor) return true;
	}
	return false;
}

void AMyCharacter::ApplyAllItemEffects()
{
	for (auto &&Item : *Inventory)
	{
		UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToChar(this, Item->EffectsToApply, Item);
	}
}

UMyAttributeSet* AMyCharacter::GetAttributes()
{
	return AttributeSetBase;
}

void AMyCharacter::ApplyKnockBack(AActor* SourceActor, float Power)
{
	if (SourceActor)
	{
		/*FVector A = FVector(GetActorLocation().X, GetActorLocation().Y, 0.0f);
		FVector B = FVector(SourceActor->GetActorLocation().X, SourceActor->GetActorLocation().Y, 0.0f);
		KnockBackVector = (A - B).GetSafeNormal() * Power;*/
		KnockBackVector = SourceActor->GetActorForwardVector().GetSafeNormal() * Power;
	}
	else
	{
		KnockBackVector = GetActorForwardVector().GetSafeNormal() * Power;
	}
	
	// FVector KnockBackVector = (GetActorLocation() - SourceActor->GetActorLocation()).GetSafeNormal() * 400.0f;
	// GetMovementComponent()->Velocity = KnockBackVector;
	StartBackslide(KnockBackVector);
}

void AMyCharacter::ApplyLaunchBack(AActor* SourceActor, FVector Power)
{
	FVector A = FVector(GetActorLocation().X, GetActorLocation().Y, 0.0f);
	FVector B = FVector(SourceActor->GetActorLocation().X, SourceActor->GetActorLocation().Y, 0.0f);
	//FVector LaunchDir = (A - B).GetSafeNormal();
	FVector LaunchDir = FVector();
	if (SourceActor) LaunchDir = SourceActor->GetActorForwardVector();
	else LaunchDir = GetActorForwardVector();
	FRotator DRot = LaunchDir.Rotation();
	FRotator PRot = Power.Rotation();
	float Angle = DRot.Yaw - PRot.Yaw;
	Power = Power.RotateAngleAxis(Angle, FVector(0.f, 0.f, 1.f));
	// Power = FVector(0.f, 600.f, 300.f);

	/*if (SourceActor) Power = -SourceActor->GetActorForwardVector() * Power;
	else Power = GetActorForwardVector().GetSafeNormal() * Power;*/
	//UE_LOG(LogTemp, Warning, TEXT("FV: %s, power: %s"), *SourceActor->GetActorForwardVector().ToString(), *Power.ToString());
	LastLaunchBack = Power;
	// LaunchDir.Z = Power.Z;
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AMyCharacter::OnDelayedLaunch, 0.05f, false);
	
	TSubclassOf<UGameplayEffect> NoControlEffect = GetMyGameInstance()->NoControlEffectRef;
	const FGameplayEffectSpecHandle Handle = AbilitySystem->MakeOutgoingSpec(NoControlEffect, 0.f, AbilitySystem->MakeEffectContext());
	FGameplayTag NoControlTag = FGameplayTag::RequestGameplayTag(TEXT("data.nocontrol"));
	Handle.Data.Get()->SetSetByCallerMagnitude(NoControlTag, 0.5f);
	AbilitySystem->ApplyGameplayEffectSpecToSelf(*(Handle.Data.Get()));
	// UE_LOG(LogTemp, Warning, TEXT("Applying Launch: %s"), *Power.ToString());
}

void AMyCharacter::OnDelayedLaunch()
{
	LaunchCharacter(LastLaunchBack, true, true);

}

FTransform AMyCharacter::GetProjectileSpawn()
{
	return SpawnArrow->GetComponentTransform();
	// UE_LOG(LogTemp, Warning, TEXT("Char casting projectile"));
}

bool AMyCharacter::HasControl()
{
	FGameplayTag HitStunTag = FGameplayTag::RequestGameplayTag(TEXT("status.hitstun"));
	FGameplayTag NoControlTag = FGameplayTag::RequestGameplayTag(TEXT("status.nocontrol"));
    if(AbilitySystem->HasMatchingGameplayTag(HitStunTag) || AbilitySystem->HasMatchingGameplayTag(NoControlTag))
	{
		return false;
	}
	return bHasControl;
}


void AMyCharacter::SetAggroTarget(APawn* NewTarget)
{
	IGetHit* NewChar = Cast<IGetHit>(NewTarget);
	if (!NewChar || !NewChar->IsAlive() || !IsAlive()) return;
	if (NewTarget->IsPlayerControlled())
	{
		AAIController* AiCont = Cast<AAIController>(GetController());
		if (!ensure(AiCont != nullptr)) return;
		UBlackboardComponent* MyBB = AiCont->GetBlackboardComponent();
		if (!ensure(MyBB != nullptr)) return;
		auto OldTarget = MyBB->GetValueAsObject(FName(TEXT("TargetChar")));
		if (OldTarget) return;
		MyBB->SetValueAsObject(FName(TEXT("TargetChar")), NewTarget);
		// UE_LOG(LogTemp, Warning, TEXT("Seen %s"), *SeenPawn->GetName());
		SetTargetEnemy(NewTarget);
	}

	UMyGameInstance* MyGI = Cast<UMyGameInstance>(GetGameInstance());
	if (!MyGI) return;
	ALevelBuilder* Builder = MyGI->GetLevelBuilder();
	if (!Builder) return;
	Builder->AggroRoom(NewTarget, GetActorLocation());
	if (AggroSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), AggroSound, GetActorLocation());
}

void AMyCharacter::SetIsInCombat(bool NewState)
{
	if (!InCombatBuff || !AbilitySystem) return;
	if (NewState)
	{
		const FGameplayEffectSpecHandle CombatBuffHandle = AbilitySystem->MakeOutgoingSpec(InCombatBuff, 0.f, AbilitySystem->MakeEffectContext());
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*(CombatBuffHandle.Data.Get()));
		SetRunning(false);
		// if I just got in combat, then also set the RUN button (101) to inactive and set its cooldown to the duration of the combat buff
		AMyPlayerController* MyCont = Cast<AMyPlayerController>(GetController());
		if (MyCont)
		{
			MyCont->SetAbilityKeyDown(101, false);
			MyCont->ShowAbilityCooldown(101, TimeRequiredToRun + CombatBuffHandle.Data.Get()->GetDuration());
		}
	}
	else
	{
		AbilitySystem->RemoveActiveGameplayEffectBySourceEffect(InCombatBuff, AbilitySystem);
	}
}

bool AMyCharacter::IsInCombat()
{
	if (AbilitySystem && AbilitySystem->GetGameplayEffectCount(InCombatBuff, AbilitySystem) > 0)
	{
		return true;
	}
	return false;
}

void AMyCharacter::SetRunning(bool NewState)
{
	if (!RunBuff || !AbilitySystem) return;
	if (NewState)
	{
		const FGameplayEffectSpecHandle Handle = AbilitySystem->MakeOutgoingSpec(RunBuff, 0.f, AbilitySystem->MakeEffectContext());
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*(Handle.Data.Get()));
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->RotationRate = RunRotationRate;
			GetCharacterMovement()->MaxAcceleration = RunAccel;
		}
		AMyPlayerController* MyCont = Cast<AMyPlayerController>(GetController());
		if (MyCont)
		{
			MyCont->SetAbilityKeyDown(101, true);
		}
	}
	else
	{
		AbilitySystem->RemoveActiveGameplayEffectBySourceEffect(RunBuff, AbilitySystem);
		TimeHoldingRun = 0.f;
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->RotationRate = WalkRotationRate;
			GetCharacterMovement()->MaxAcceleration = WalkAccel;
		}
		//AMyPlayerController* MyCont = Cast<AMyPlayerController>(GetController());
		//if (MyCont)
		//{
		//	MyCont->SetAbilityKeyDown(101, false);
		//	//MyCont->ShowAbilityCooldown(101, TimeRequiredToRun + Handle.Data.Get()->GetDuration());
		//}
	}
}

bool AMyCharacter::IsRunning()
{
	if (AbilitySystem && AbilitySystem->GetGameplayEffectCount(RunBuff, AbilitySystem) > 0)
	{
		return true;
	}
	return false;
}

/// <summary>
/// Used by bots to walk slowly than normal speed while strafing around player in non aggressive AI mode.	
/// </summary>
/// <param name="NewState">Whether to activate walking or disable it</param>
void AMyCharacter::SetWalking(bool NewState)
{
	if (NewState && WalkBuff && AbilitySystem)
	{
		const FGameplayEffectSpecHandle Handle = AbilitySystem->MakeOutgoingSpec(WalkBuff, 0.f, AbilitySystem->MakeEffectContext());
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*(Handle.Data.Get()));
		SetRunning(false);
	}
	if (!NewState && WalkBuff && AbilitySystem)
	{
		AbilitySystem->RemoveActiveGameplayEffectBySourceEffect(WalkBuff, AbilitySystem);
	}
}

bool AMyCharacter::IsWalking()
{
	if (AbilitySystem && AbilitySystem->GetGameplayEffectCount(WalkBuff, AbilitySystem) > 0)
	{
		return true;
	}
	return false;
}

FDieSignature& AMyCharacter::GetReportDeathDelegate()
{
	return OnDieDelegate;
}

void AMyCharacter::CheckWalls()
{
	UGameInstance* GI = GetGameInstance();
	UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
	if (!MyGI) return;
	ALevelBuilder* Builder = MyGI->GetLevelBuilder();
	if (!Builder) return;
	Builder->OnUpdateCharCoord(GetActorLocation());
	// AStaticMeshActor* SM = Builder->GetBottomWallFromLoc(GetActorLocation());
	// if (SM && SM->GetStaticMeshComponent())
	// {
	// 	// UE_LOG(LogTemp, Warning, TEXT("wall is %s"), *SM->GetName());
	// 	SM->GetStaticMeshComponent()->SetVisibility(false);
	// }
	// else
	// {
	// 	// UE_LOG(LogTemp, Warning, TEXT("no wall"));
	// }

}

void AMyCharacter::SetBodyColor(FLinearColor NewColor)
{
	// BodyColor = NewColor;
	if (DynaMat) DynaMat->SetVectorParameterValue(FName("BodyColor"), NewColor);
}
void AMyCharacter::ResetBodyColor()
{
	if (DynaMat) DynaMat->SetVectorParameterValue(FName("BodyColor"), BodyColor);
}

void AMyCharacter::SetGlow(float NewGlow)
{
	if (DynaMat) DynaMat->SetScalarParameterValue(FName("GlowAlpha"), NewGlow);
}

void AMyCharacter::ResetGlow()
{
	if (DynaMat) DynaMat->SetScalarParameterValue(FName("GlowAlpha"), 0.f);
}
