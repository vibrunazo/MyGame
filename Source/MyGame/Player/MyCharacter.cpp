// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MyCharacter.h"
#include "MyPlayerController.h"
#include "../MyGameInstance.h"
#include "../Abilities/LootComponent.h"
#include "../Level/LevelBuilder.h"
#include "../Props/ItemDataAsset.h"
#include "../Props/Pickup.h"
#include "../UI/MyHealthBar.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
// #include "../Abilities/MyAttributeSet.h"
#include "MyAnimInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/PawnSensingComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/ArrowComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

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
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 700.0f; // The camera follows at this distance behind the character	
	// CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->SetWorldRotation(FRotator(-40.0f, 0.0f, 0.0f));
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 2.0f;
	CameraBoom->CameraLagMaxDistance = 200.0f;
	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	HealthBarComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComponent"));
	HealthBarComp->SetupAttachment(RootComponent);

	SpawnArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Arrow"));
	SpawnArrow->SetupAttachment(RootComponent);
	SpawnArrow->SetRelativeLocation(FVector(50.0f, 0.0f, 10.0f));
	SpawnArrow->bTreatAsASprite = true;
	
	PawnSenseComp = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("Pawn Sensing"));
	PawnSenseComp->OnSeePawn.AddDynamic(this, &AMyCharacter::OnPawnSeen);

	// Our ability system component.
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSetBase = CreateDefaultSubobject<UMyAttributeSet>(TEXT("AttributeSetBase"));
	LootComponent = CreateDefaultSubobject<ULootComponent>(TEXT("Loot Component"));

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

	HealthBarComp->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
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
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMyCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

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
	AttributeSetBase->SetHealth(MaxHealth);
	AttributeSetBase->SetAttack(Attack);
	AttributeSetBase->SetDefense(Defense);
	BaseSpeed = GetCharacterMovement()->MaxWalkSpeed;
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetSpeedAttribute()).AddUObject(this, &AMyCharacter::OnSpeedChange);
	UpdateHealthBar();
	// if (IsPlayerControlled()) 
	// {
	// 	Team = 1;
	// }
		// else Team = 0;
	DynaMat = GetMesh()->CreateDynamicMaterialInstance(0);
	ResetBodyColor();
	if (IsPlayerControlled() && GetMyGameInstance() && AttributeSetBase)
	{
		AttributeSetBase->SetHealth(GetMyGameInstance()->Health);
		UpdateHealthBar();
		GetMyGameInstance()->SetCharRef(this);
		Inventory = &GetMyGameInstance()->Inventory;
		ApplyAllItemEffects();
	}
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
	}
	if (IsPlayerControlled())
	{
		CheckWalls();
		CalculateDash();
	}

}

void AMyCharacter::CalculateDash()
{
	float length = 0.0f;
	length = FMath::Abs(RightAxis)*FMath::Abs(RightAxis) + FMath::Abs(ForwardAxis)*FMath::Abs(ForwardAxis);
	length = FMath::Sqrt(length);
	if (length >= 0.89f)
	{
		// float CurAngle = GetInputAngle();
		// float Delta = FMath::FindDeltaAngleDegrees(CurAngle, 45.0f);
		FVector CurVector = FVector(ForwardAxis, RightAxis, 0.0f);
		if (GetWorld()->GetTimeSeconds() - LastInputApexTime < DoubleTapDelay && LastInputZeroTime > LastInputApexTime)
		{
			float cos = CurVector.CosineAngle2D(LastInputVector);
			float acos = UKismetMathLibrary::DegAcos(cos);
			// UE_LOG(LogTemp, Warning, TEXT("Forward: %f, Right: %f, len: %f, angle: %f"), ForwardAxis, RightAxis, length, acos);
			if (acos <= 20)
			{
				// UE_LOG(LogTemp, Warning, TEXT("Yes dash"));
				ActivateAbilityByEvent("dash");
			}
		}
		LastInputVector = CurVector;
		LastInputApexTime = GetWorld()->GetTimeSeconds();
	}
	else
	{
		LastInputZeroTime = GetWorld()->GetTimeSeconds();
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
	Super::Jump();
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
	UUserWidget* Widget = HealthBarComp->GetUserWidgetObject();
	UMyHealthBar* HealthBar = Cast<UMyHealthBar>(Widget);
	if (HealthBar && AttributeSetBase) HealthBar->SetHealth(AttributeSetBase->GetHealth());
	else {UE_LOG(LogTemp, Warning, TEXT("UpdateHealthbar: Failed on %s"), *GetName());}
	// OnUpdatedHealth.Broadcast(AttributeSetBase->GetHealth());
	// UE_LOG(LogTemp, Warning, TEXT("HP: %f"), AttributeSetBase->GetHealth());
}

void AMyCharacter::OnSpeedChange(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * AttributeSetBase->GetSpeed();
	// UE_LOG(LogTemp, Warning, TEXT("My Speed changed to %f"), GetCharacterMovement()->MaxWalkSpeed);
}

FActiveGameplayEffectHandle* AMyCharacter::OnGetHitByEffect(FGameplayEffectSpecHandle NewEffect, AActor* SourceActor)
{
	// UE_LOG(LogTemp, Warning, TEXT("Char getting effected"));
	FGameplayTagContainer EffectTags;
	// NewEffect.Data->GetAllGrantedTags(EffectTags);
	if (!NewEffect.Data || !AbilitySystem) { return nullptr; }
	NewEffect.Data->GetAllAssetTags(EffectTags);
	// const FActiveGameplayEffect* AGE = AbilitySystem->GetActiveGameplayEffect(NewEffect);
	// FGameplayTag HitstunTag = FGameplayTag::RequestGameplayTag(TEXT("status.hitstun"));
	FGameplayTag HitstunTag = FGameplayTag::RequestGameplayTag(TEXT("data.hitstun"));
	FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("data.knockback")); 
	FGameplayTag LaunchTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch")); 
	FGameplayTag LaunchXTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.x")); 
	FGameplayTag LaunchYTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.y")); 
	FGameplayTag LaunchZTag = FGameplayTag::RequestGameplayTag(TEXT("data.launch.z")); 
	// {{TagName="data.knockback" },500.000000}
	if (EffectTags.HasTag(HitstunTag)) 
	{
		// UE_LOG(LogTemp, Warning, TEXT("Has Hitstun Tag, Count: %d, Immune: %d"), HitStunCount, StunImmune);
		if (HasStunImmune()) return nullptr;
		else IncrementHitStunCount();
	}
	if (EffectTags.HasTag(KnockbackTag)) 
	// if (EffectTags.HasTag(KnockbackTag) && !StunImmune) 
	{
		float Knockback = NewEffect.Data.Get()->GetSetByCallerMagnitude(KnockbackTag);
		// TMap<FGameplayTag, float> KnockbackMap = NewEffect.Data.Get()->SetByCallerTagMagnitudes;
		// float Knockback = *(KnockbackMap.Find(KnockbackTag));
		// UE_LOG(LogTemp, Warning, TEXT("Has KnockbackTag, Knockback: %f"), Knockback);
		ApplyKnockBack(SourceActor, Knockback);
	}
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
	FActiveGameplayEffectHandle ActiveEffect = AbilitySystem->ApplyGameplayEffectSpecToSelf(*(NewEffect.Data.Get()));
	// FActiveGameplayEffectHandle* ActiveEffectPointer = &ActiveEffect;
	UpdateHealthBar();
	return new FActiveGameplayEffectHandle(ActiveEffect);
}

/**Sets the outline of the enemy, visible through walls, when the enemy is hit
 * 	The Post process material looks for a custom depth of stencil value 2 or more to draw the outline.
 *  2 is the player outline color, greater is the enemy
 */
void AMyCharacter::SetOutline()
{
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(3);
	GetWorldTimerManager().SetTimer(OutlineTimer, this, &AMyCharacter::RemoveOutline, 5.f, false);
}

/* Removes the outline of an enemy character by setting the custom depth stencil back to zero.
Called from a timer set by SetOutline() and by the OnDie() event */
void AMyCharacter::RemoveOutline()
{
	GetMesh()->SetRenderCustomDepth(false);
	GetMesh()->SetCustomDepthStencilValue(0);
}

void AMyCharacter::IncrementHitStunCount()
{
	// if (StunImmune || GetWorld()->GetTimeSeconds() > LastHitstunTime + StunImmuneCooldown) {HitStunCount = 0; StunImmune = false;}
	HitStunCount++;
	// LastHitstunTime = GetWorld()->GetTimeSeconds();
	if (MaxStuns > 0 && HitStunCount >= MaxStuns)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s is Stunimmune"), *GetName());
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

void AMyCharacter::OnDamaged(AActor* SourceActor)
{
	// if (!ensure(GetHitMontage != nullptr)) return;
	if (!GetHitMontage)
	{
		UAnimInstance* Anim = GetMesh()->GetAnimInstance();
		UMyAnimInstance* MyAnim = Cast<UMyAnimInstance>(Anim);
		if (!ensure(MyAnim != nullptr)) return;
		GetHitMontage = MyAnim->GetHitMontage;
	}
	// UE_LOG(LogTemp, Warning, TEXT("I was damaged"));
	PlayAnimMontage(GetHitMontage);
	UGameplayStatics::PlayWorldCameraShake(GetWorld(), GetCamShake(), GetActorLocation(), 0.0f, CamShakeRange);
	if (!IsPlayerControlled())
	{
		APawn* SeenPawn = Cast<APawn>(SourceActor);
		if (!SeenPawn) return;
		SetAggroTarget(SeenPawn);
		SetOutline();
	}
	
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
	UWorld* World = GetWorld();
	// FTimerManager TM = FTimerManager::FTimerManager;
	FTimerHandle Handle;
	FTimerHandle Handle2;
	GetWorldTimerManager().SetTimer(Handle, this, &AMyCharacter::OnDelayedDeath, 5.0f, false);
	GetWorldTimerManager().SetTimer(Handle2, this, &AMyCharacter::OnDelayedLaunch2, .05f, false);
	bHasControl = false;
	DisableInput(nullptr);
	HealthBarComp->SetVisibility(false);
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
	if (MyAnim)
	{
		// MyAnim->StartRagdoll();
	}
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
	FVector Loc = GetActorLocation();
    FActorSpawnParameters params;
    params.bNoFail = true;
    params.Instigator = this;
    params.Owner = this;
	if (!LootComponent) return;
	auto LootTable = LootComponent->LootTable;
	for (auto &&Loot : LootTable)
	{
		if (!Loot.Item) continue;
    	APickup* NewPickup = GetWorld()->SpawnActor<APickup>(Loot.Item->PickupActor, Loc, FRotator::ZeroRotator, params);
		NewPickup->SetItemData(Loot.Item);
		// UE_LOG(LogTemp, Warning, TEXT("dropped a %s"), *Loot.Pickup->GetName());
		// if (NewPickup) {UE_LOG(LogTemp, Warning, TEXT("Spawned"));}
		// else {UE_LOG(LogTemp, Warning, TEXT("Nope"));}
	}
	
}

bool AMyCharacter::IsAlive()
{
	return AttributeSetBase->GetHealth() > 0;
}

bool AMyCharacter::IsValidLowLevel()
{
	return Super::IsValidLowLevel();
}

void AMyCharacter::OnHitPause(float Duration)
{
	CustomTimeDilation = 0.01f;
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AMyCharacter::OnHitPauseEnd, Duration, false);
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

void AMyCharacter::OnHitPauseEnd()
{
	CustomTimeDilation = 1.0f;
}

void AMyCharacter::OnPawnSeen(APawn* SeenPawn)
{
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
	FVector A = FVector(GetActorLocation().X, GetActorLocation().Y, 0.0f);
	FVector B = FVector(SourceActor->GetActorLocation().X, SourceActor->GetActorLocation().Y, 0.0f);
	KnockBackVector = (A - B).GetSafeNormal() * Power;
	// FVector KnockBackVector = (GetActorLocation() - SourceActor->GetActorLocation()).GetSafeNormal() * 400.0f;
	// GetMovementComponent()->Velocity = KnockBackVector;
	StartBackslide(KnockBackVector);
}

void AMyCharacter::ApplyLaunchBack(AActor* SourceActor, FVector Power)
{
	FVector A = FVector(GetActorLocation().X, GetActorLocation().Y, 0.0f);
	FVector B = FVector(SourceActor->GetActorLocation().X, SourceActor->GetActorLocation().Y, 0.0f);
	FVector LaunchDir = (A - B).GetSafeNormal();
	FRotator DRot = LaunchDir.Rotation();
	FRotator PRot = Power.Rotation();
	float Angle = DRot.Yaw - PRot.Yaw;
	Power = Power.RotateAngleAxis(Angle, FVector(0.f, 0.f, 1.f));
	// Power = FVector(0.f, 600.f, 300.f);
	LastLaunchBack = Power;
	// LaunchDir.Z = Power.Z;
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AMyCharacter::OnDelayedLaunch, 0.05f, false);
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
	if (!NewTarget) return;
	if (NewTarget->IsPlayerControlled())
	{
		AAIController* AiCont = Cast<AAIController>(GetController());
		if (!ensure(AiCont != nullptr)) return;
		UBlackboardComponent* MyBB = AiCont->GetBlackboardComponent();
		if (!ensure(MyBB != nullptr)) return;
		MyBB->SetValueAsObject(FName(TEXT("TargetChar")), NewTarget);
		// UE_LOG(LogTemp, Warning, TEXT("Seen %s"), *SeenPawn->GetName());
	}
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