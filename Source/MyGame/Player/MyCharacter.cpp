// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MyCharacter.h"
// #include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
// #include "../Abilities/MyAttributeSet.h"
#include "../UI/MyHealthBar.h"
#include "MyAnimInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/PawnSensingComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/ArrowComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "../MyGameInstance.h"
#include "MyPlayerController.h"
#include "../Level/LevelBuilder.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/KismetMathLibrary.h"

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

	SetDefaultProperties();
}

void AMyCharacter::SetDefaultProperties()
{
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -97.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

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
	try
	{
		Super::BeginPlay();
		if(AbilitySystem)
		{
			for (auto &&Ability : Abilities)
			{
				GiveAbility(Ability.AbilityClass);
			}
		}
		if (!ensure(AttributeSetBase != nullptr)) return;
		AttributeSetBase->SetMaxHealth(MaxHealth);
		AttributeSetBase->SetHealth(MaxHealth);
		UpdateHealthBar();
		// if (IsPlayerControlled()) 
		// {
		// 	Team = 1;
		// }
		// else Team = 0;
	}
	catch(const std::exception& e)
	{
		UE_LOG(LogTemp, Error, TEXT("ERROR ON BEGINPLAY: %s"), *e.what());
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
		if (GetWorld()->GetTimeSeconds() - LastInputApexTime < 0.5f && LastInputZeroTime > LastInputApexTime)
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
				try
				{
					AbilitySystem->TryActivateAbilityByClass(Ability.AbilityClass, true);
				}
				catch(const std::exception& e)
				{
					UE_LOG(LogTemp, Error, TEXT("Error trying to activate ability: "), *e.what());
				}
				
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

void AMyCharacter::OnGetHitByEffect(FGameplayEffectSpecHandle NewEffect)
{
	// UE_LOG(LogTemp, Warning, TEXT("Char getting effected"));
	FGameplayTagContainer EffectTags;
	NewEffect.Data->GetAllGrantedTags(EffectTags);
	// const FActiveGameplayEffect* AGE = AbilitySystem->GetActiveGameplayEffect(NewEffect);
	FGameplayTag HitstunTag = FGameplayTag::RequestGameplayTag(TEXT("status.hitstun"));
	if (EffectTags.HasTag(HitstunTag)) 
	{
		// UE_LOG(LogTemp, Warning, TEXT("Has Hitstun Tag, Count: %d, Immune: %d"), HitStunCount, StunImmune);
		if (StunImmune && GetWorld()->GetTimeSeconds() < LastHitstunTime + StunImmuneCooldown) return;
		else IncrementHitStunCount();
	}
	AbilitySystem->ApplyGameplayEffectSpecToSelf(*(NewEffect.Data.Get()));
	UpdateHealthBar();
}

void AMyCharacter::IncrementHitStunCount()
{
	if (StunImmune || GetWorld()->GetTimeSeconds() > LastHitstunTime + StunImmuneCooldown) {HitStunCount = 0; StunImmune = false;}
	HitStunCount++;
	LastHitstunTime = GetWorld()->GetTimeSeconds();
	if (MaxStuns > 0 && HitStunCount >= MaxStuns) StunImmune = true;
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
	APawn* SeenPawn = Cast<APawn>(SourceActor);
	if (!SeenPawn) return;
	SetAggroTarget(SeenPawn);
	FVector A = FVector(GetActorLocation().X, GetActorLocation().Y, 0.0f);
	FVector B = FVector(SourceActor->GetActorLocation().X, SourceActor->GetActorLocation().Y, 0.0f);
	KnockBackVector = (A - B).GetSafeNormal() * 400.0f;
	// FVector KnockBackVector = (GetActorLocation() - SourceActor->GetActorLocation()).GetSafeNormal() * 400.0f;
	// GetMovementComponent()->Velocity = KnockBackVector;
	StartBackslide(KnockBackVector);
	
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
	GetWorldTimerManager().SetTimer(Handle, this, &AMyCharacter::OnDelayedDeath, 5.0f, false);
	HealthBarComp->SetVisibility(false);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->AddForce(FVector(-80000.0f, 0.0f, 200000.0f), NAME_None, true);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	bHasControl = false;
	DisableInput(nullptr);
	AMyPlayerController* MyCont = Cast<AMyPlayerController>(GetController());
	if (MyCont)
	{
		MyCont->OnCharDies(this);
	}
	else DetachFromControllerPendingDestroy();
}

void AMyCharacter::OnDelayedDeath()
{
	AMyPlayerController* MyCont = Cast<AMyPlayerController>(GetController());
	if (MyCont)
	{
		MyCont->OnDelayedCharDies(this);
	}
	Destroy();
}

bool AMyCharacter::IsAlive()
{
	return AttributeSetBase->GetHealth() > 0;
}

void AMyCharacter::OnHitPause(float Duration)
{
	CustomTimeDilation = 0.01f;
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AMyCharacter::OnHitPauseEnd, Duration, false);
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
	Builder->HideWall(GetActorLocation());
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


