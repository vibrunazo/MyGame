// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MyCharacter.h"
#include "../MyGameplayTags.h"
#include "EnemyCharBase.h"
#include "MyPlayerController.h"
#include "../MyGameInstance.h"
#include "../Abilities/LootComponent.h"
#include "../Level/LevelBuilder.h"
#include "../Props/ItemDataAsset.h"
#include "../Props/LearnItemDataAsset.h"
#include "../Props/Pickup.h"
#include "../UI/MyHealthBar.h"
#include "../Abilities/HitReactionEffectComponent.h"

//#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
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
	TargetDetection->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
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
	// Hitboxes ride on bones, so bones must update even when the mesh isn't rendered (off-screen enemies, headless tests)
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	// DynaMat = GetMesh()->CreateDynamicMaterialInstance(0);
	// if (DynaMat) DynaMat->SetVectorParameterValue(FName("BodyColor"), BodyColor);

	HealthBarComp->SetRelativeLocation(FVector(0.0f, 0.0f, -140.0f));
	HealthBarComp->SetWidgetSpace(EWidgetSpace::Screen);

	// Soft paths only: loading Blueprints from a native constructor deadlocks UE5's loader.
	// Resolved in OnConstruction / PostInitializeComponents.
	DefaultAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Anims/ABP_CharAnim.ABP_CharAnim_C")));
	DefaultAIControllerClass = TSoftClassPtr<AController>(FSoftObjectPath(TEXT("/Game/Blueprints/AI/BP_AIController.BP_AIController_C")));
	Team = 0;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMyCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Buttons and movement are Enhanced Input actions bound by AMyPlayerController; touch still jumps
	check(PlayerInputComponent);
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMyCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMyCharacter::TouchStopped);
}

void AMyCharacter::SetMoveInput(FVector2D Value)
{
	MoveForward(Value.X);
	MoveRight(Value.Y);
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
	if ((Controller != NULL) && (Value != 0.0f) && HasMoveControl())
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
	if ( (Controller != NULL) && (Value != 0.0f)  && HasMoveControl())
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

void AMyCharacter::PostInitializeComponents()
{
	// Still the engine default means this Blueprint didn't choose a controller; must be set before Super spawns it
	if (AIControllerClass == GetDefault<APawn>()->AIControllerClass && !DefaultAIControllerClass.IsNull())
	{
		AIControllerClass = DefaultAIControllerClass.LoadSynchronous();
	}
	Super::PostInitializeComponents();
}

UAbilitySystemComponent* AMyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
};

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if(!AbilitySystem) return;
	AbilitySystem->InitAbilityActorInfo(this, this);
	AbilitySystem->RegisterGameplayTagEvent(MyGameplayTags::Status_NoPawnBlock, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMyCharacter::PawnBlockTagChanged);
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
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetRotSpeedAttribute()).AddUObject(this, &AMyCharacter::OnRotSpeedChange);
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
	RefreshPawnCollision();
}


void AMyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Held slots retry every frame (player buttons and AI presses alike): holding a button repeats the
	// move, and a press during another attack fires as soon as that attack can be cancelled
	if (InputEnabled())
	{
		for (uint8 Slot = 0; Slot < IsAbilityKeyDown.Num(); ++Slot)
		{
			if (IsAbilityKeyDown[Slot]) ActivateAbilityByInput(Slot);
		}
	}
	if (IsPlayerControlled())
	{
		CheckWalls();
		UpdateRun(DeltaSeconds);
	}
}

void AMyCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!GetMesh()->GetAnimClass() && !DefaultAnimClass.IsNull())
	{
		GetMesh()->SetAnimInstanceClass(DefaultAnimClass.LoadSynchronous());
	}
	DynaMat = GetMesh()->CreateDynamicMaterialInstance(0);
	ResetBodyColor();
}

/// <summary>
/// Running from the move input: holding a direction out of combat starts running, a quick tap in another direction stops it
/// </summary>
void AMyCharacter::UpdateRun(float DeltaSeconds)
{
	const FVector CurVector = FVector(ForwardAxis, RightAxis, 0.0f);
	const float Now = GetWorld()->GetTimeSeconds();
	if (CurVector.Size2D() < DoubleTapAxisDepth)
	{
		LastInputZeroTime = Now;
		return;
	}
	TryRun(DeltaSeconds);
	// LastInputZeroTime > LastInputApexTime: the stick came back from neutral, so this is a new tap
	if (LastInputZeroTime > LastInputApexTime)
	{
		if (Now - LastInputApexTime < DoubleTapDelay && UKismetMathLibrary::DegAcos(CurVector.CosineAngle2D(LastInputVector)) > 20.f)
		{
			SetRunning(false);
		}
		LastInputVector = CurVector;
		LastInputApexTime = Now;
	}
}

void AMyCharacter::Dash(FVector2D Direction)
{
	// ground only, like its old ability entry (ground yes, air no)
	if (!HasControl() || !AbilitySystem || GetCharacterMovement()->IsFalling()) return;
	if (GetController() && !Direction.IsNearlyZero()) GetController()->SetControlRotation(FVector(Direction, 0.f).Rotation());
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, MyGameplayTags::Event_Dash, FGameplayEventData());
	if (AMyPlayerController* MyCont = Cast<AMyPlayerController>(GetController()))
	{
		MyCont->UpdateHUDAbilityKey(EInput::Dash, true, 0.5f);
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
	// the same class can be learned again (another item with it); one spec is enough
	if (HasAuthority() && Ability && !AbilitySystem->FindAbilitySpecFromClass(Ability))
	{
		AbilitySystem->GiveAbility(FGameplayAbilitySpec(Ability.GetDefaultObject(), 1, 0));
	}
}

void AMyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// refresh the ASC's actor info (player controller) now that we have a controller
	if (AbilitySystem) AbilitySystem->InitAbilityActorInfo(this, this);
}

void AMyCharacter::LearnAbility(FAbilityStruct Ability)
{
	if (!Ability.AbilityClass)
	{
		UE_LOG(LogTemp, Error, TEXT("trying to learn ability but no ability class found on struct"));
		return;
	}
	FindAndRemoveOverlappingAbilities(Ability);
	UE_LOG(LogTemp, Warning, TEXT("Learning Ability: %s"), *Ability.AbilityClass->GetName());
	Abilities.Add(Ability);
	GiveAbility(Ability.AbilityClass);
	AMyPlayerController* MyCont = GetController<AMyPlayerController>();
	if (!MyCont) return;
	MyCont->UpdateHUDAbility(Ability, true);
		//UMyHUDWidget* HUDWidgetRef
}

void AMyCharacter::LearnAbilities(TArray<struct FAbilityStruct> NewAbilities)
{
	for (auto&& Ability : NewAbilities)
	{
		LearnAbility(Ability);
	}
}

/// <summary>
/// Look into the array list of learned Abilities for any Ability that overlaps the given AbilityToCompare. That is,
/// a learned ability that has the same input slot as the given AbilityToCompare.
/// Then removes the abilities found from the Abilities Array.
/// </summary>
/// <param name="AbilityToCompare">The given ability to compare its inputs to all learned abilities</param>
void AMyCharacter::FindAndRemoveOverlappingAbilities(FAbilityStruct AbilityToCompare)
{
	TArray<FAbilityStruct> AbilitiesToRemove;
	for (auto&& LearnedAbility : Abilities)
	{
		// same slot, and both air moves or both ground moves (an air punch doesn't replace the ground punch)
		if (LearnedAbility.Input == AbilityToCompare.Input && IsAirAbility(LearnedAbility.AbilityClass) == IsAirAbility(AbilityToCompare.AbilityClass))
		{
			AbilitiesToRemove.Add(LearnedAbility);
		}
	}
	for (auto&& OldAbility : AbilitiesToRemove)
	{
		UE_LOG(LogTemp, Warning, TEXT("Removing Overlapping Ability: %s"), *OldAbility.AbilityClass->GetName());
		Abilities.Remove(OldAbility);
		// the replaced ability must leave the ASC too, unless it's being re-learned or another slot uses the same class
		const bool bStillUsed = OldAbility.AbilityClass == AbilityToCompare.AbilityClass
			|| Abilities.ContainsByPredicate([&](const FAbilityStruct& A) { return A.AbilityClass == OldAbility.AbilityClass; });
		if (FGameplayAbilitySpec* Spec = bStillUsed ? nullptr : AbilitySystem->FindAbilitySpecFromClass(OldAbility.AbilityClass))
		{
			AbilitySystem->ClearAbility(Spec->Handle);
		}
	}
}

#if WITH_EDITOR
void AMyCharacter::SetAttributeBaseForTest(FGameplayAttribute Attribute, float Value)
{
	if (AbilitySystem && Attribute.IsValid()) AbilitySystem->SetNumericAttributeBase(Attribute, Value);
	UpdateHealthBar();
}
#endif

bool AMyCharacter::IsAirAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
	return AbilityClass && AbilityClass.GetDefaultObject()->GetAssetTags().HasTagExact(MyGameplayTags::Activate_FlyingAttack);
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
	AMyPlayerController* MyCont = GetController<AMyPlayerController>();
	TArray<uint8> InputsToCheck;
	InputsToCheck.Add(Index);
	if (MyCont && MyCont->GetSuperMod()) InputsToCheck.Add(Index + 10);
	if (MyCont && MyCont->GetUltraMod()) InputsToCheck.Add(Index + 20);
	TArray<FAbilityStruct> AbilitiesThatCanActivate;
	// first add all abilities that can activate from this input (ie punch) to a list
	for (auto &&Ability : Abilities)
	{
		// ground moves are blocked while airborne and air moves on the ground (OnMovementModeChanged), plus each
		// ability's own activation tags: the fallback below only considers moves that could start right now
		const UGameplayAbility* AbilityCDO = Ability.AbilityClass ? Ability.AbilityClass.GetDefaultObject() : nullptr;
		if (InputsToCheck.Contains((uint8)Ability.Input) && AbilityCDO && AbilityCDO->DoesAbilitySatisfyTagRequirements(*AbilitySystem))
		{
			AbilitiesThatCanActivate.Add(Ability);
		}
	}
	// then, activate only the highest priority ability with that button
	// ie. if pressing super mod + punch, will activate only the super punch, not regular punch
	if (AbilitiesThatCanActivate.Num() == 0) return;
	uint8 BestAbilityScore = 0;
	FAbilityStruct BestAbility;
	for (auto&& Ability : AbilitiesThatCanActivate)
	{
		if ((uint8)Ability.Input >= BestAbilityScore)
		{
			BestAbility = Ability;
			BestAbilityScore = (uint8)Ability.Input;
		}
	}
	bool Success = AbilitySystem->TryActivateAbilityByClass(BestAbility.AbilityClass, true);
	if (MyCont) MyCont->UpdateHUDAbilityKey(BestAbility.Input, true);

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
		FGameplayTag HealthTag = MyGameplayTags::Status_Health_75;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, HealthTag, FGameplayEventData());
	}
	if (NewHealthPct <= 0.7f && OldHealthPct > 0.7f)
	{
		FGameplayTag HealthTag = MyGameplayTags::Status_Health_70;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, HealthTag, FGameplayEventData());
	}
	if (NewHealthPct <= 0.5f && OldHealthPct > 0.5f)
	{
		FGameplayTag HealthTag = MyGameplayTags::Status_Health_50;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, HealthTag, FGameplayEventData());
	}
	if (NewHealthPct <= 0.3f && OldHealthPct > 0.3f)
	{
		FGameplayTag HealthTag = MyGameplayTags::Status_Health_30;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, HealthTag, FGameplayEventData());
	}
	if (NewHealthPct <= 0.25f && OldHealthPct > 0.25f)
	{
		FGameplayTag HealthTag = MyGameplayTags::Status_Health_25;
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

void AMyCharacter::OnRotSpeedChange(const FOnAttributeChangeData& Data)
{
	UpdateRotationRate();
}

// updates the Character Movement Rotation rate to the correct amount based on whether we're walking/running and on the RotSpeed Attribute
// Called everytime a buff changes our RotSpeed Attribute and every time we change between walking/running
void AMyCharacter::UpdateRotationRate()
{
	if (!AttributeSetBase || !GetCharacterMovement()) return;
	if (IsRunning()) GetCharacterMovement()->RotationRate = RunRotationRate * AttributeSetBase->GetRotSpeed();
	else GetCharacterMovement()->RotationRate = WalkRotationRate * AttributeSetBase->GetRotSpeed();
}

void AMyCharacter::OnEffectApplied(UAbilitySystemComponent* SourceComp, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	auto ActiveEffect = AbilitySystem->GetActiveGameplayEffect(ActiveEffectHandle);
	if (!ActiveEffect) return;
	float Duration = ActiveEffect->GetDuration();
	//UE_LOG(LogTemp, Warning, TEXT("On Effect Applied of duration: %f"), Duration);
	if (Duration > 1.f) AddDurationToHealthBar(Duration, EffectSpec, ActiveEffectHandle);

	auto ContextHandle = EffectSpec.GetEffectContext();
	auto Context = ContextHandle.Get();

}

void AMyCharacter::PawnBlockTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	RefreshPawnCollision();
}

// The only place that sets the capsule's response to other pawns: ignore them while dead, airborne or
// while an effect grants status.nopawnblock (moves that pass through enemies); block otherwise
void AMyCharacter::RefreshPawnCollision()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	const bool bIgnorePawns = !IsAlive()
		|| (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
		|| (AbilitySystem && AbilitySystem->HasMatchingGameplayTag(MyGameplayTags::Status_NoPawnBlock));
	if (bIgnorePawns)
	{
		GetWorldTimerManager().ClearTimer(PawnBlockRetryTimer);
		PawnBlockWaitStart = -1.f;
		Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		return;
	}
	// Blocking again while inside another character makes movement shove the capsules apart, which can push
	// one into level geometry. Wait briefly for them to separate; if they don't (an enemy standing in us),
	// step out sideways ourselves, swept so we never go through walls
	if (const ACharacter* Other = FindOverlappingCharacter())
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (PawnBlockWaitStart < 0.f) PawnBlockWaitStart = Now;
		if (Now - PawnBlockWaitStart < 0.3f)
		{
			GetWorldTimerManager().SetTimer(PawnBlockRetryTimer, this, &AMyCharacter::RefreshPawnCollision, 0.05f, false);
			return;
		}
		StepOutOf(Other);
	}
	PawnBlockWaitStart = -1.f;
	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
}

const ACharacter* AMyCharacter::FindOverlappingCharacter() const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PawnOverlap), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Capsule->GetComponentLocation(), Capsule->GetComponentQuat(),
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), Capsule->GetCollisionShape(), Params);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const ACharacter* Other = Cast<ACharacter>(Overlap.GetActor());
		// only other characters' capsules count; their meshes overlap pawns by design
		if (Other && Overlap.GetComponent() == Other->GetCapsuleComponent()) return Other;
	}
	return nullptr;
}

void AMyCharacter::StepOutOf(const ACharacter* Other)
{
	FVector Away = GetActorLocation() - Other->GetActorLocation();
	Away.Z = 0.f;
	if (!Away.Normalize()) Away = -GetActorForwardVector().GetSafeNormal2D();
	const float Needed = GetCapsuleComponent()->GetScaledCapsuleRadius() + Other->GetCapsuleComponent()->GetScaledCapsuleRadius()
		- FVector::Dist2D(GetActorLocation(), Other->GetActorLocation()) + 2.f;
	if (Needed > 0.f) SetActorLocation(GetActorLocation() + Away * Needed, true);
}

FActiveGameplayEffectHandle AMyCharacter::OnGetHitByEffect(FGameplayEffectSpecHandle NewEffect)
{
	if (!ensure(NewEffect.Data) || !ensure(AbilitySystem)) return FActiveGameplayEffectHandle();
	// hitstun, knockback, launch and camera shake happen through the effects' Hit Reaction components
	FActiveGameplayEffectHandle ActiveEffect = AbilitySystem->ApplyGameplayEffectSpecToSelf(*NewEffect.Data.Get());
	UpdateHealthBar();
	return ActiveEffect;
}

void AMyCharacter::OnHitReaction(EHitReaction Reaction, const FGameplayEffectSpec& Spec)
{
	AActor* SourceActor = Spec.GetEffectContext().GetEffectCauser();
	switch (Reaction)
	{
	case EHitReaction::HitStun:
		// the effect can't apply while stun immune, so getting here means the stun landed
		PlayAnimMontage(GetHitMontage);
		IncrementHitStunCount();
		break;
	case EHitReaction::Knockback:
		ApplyKnockBack(SourceActor, Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_Knockback, false));
		break;
	case EHitReaction::Launch:
		ApplyLaunchBack(SourceActor, FVector(Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_Launch_X, false),
			Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_Launch_Y, false), Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_Launch_Z, false)));
		break;
	case EHitReaction::CameraShake:
		if (GetMyGameInstance()) GetMyGameInstance()->DoCamShake(Spec.GetSetByCallerMagnitude(MyGameplayTags::Data_CamShake, false));
		break;
	}
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
		FGameplayTag StunImmuneTag = MyGameplayTags::Data_StunImmune;
		Handle.Data.Get()->SetSetByCallerMagnitude(StunImmuneTag, StunImmuneCooldown);
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*(Handle.Data.Get()));
	}
}

/* Returns true if the Character is immune to stun. Checks if the Char has the Gameplay Tag status.stunimmune */
bool AMyCharacter::HasStunImmune()
{
	FGameplayTag ImmuneTag = MyGameplayTags::Status_StunImmune;
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
	//UE_LOG(LogTemp, Warning, TEXT("damage effect, tags: %s"), *tags.ToString()); 
	/*FGameplayTag HitStun = MyGameplayTags::Data_HitStun;
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
	RefreshPawnCollision();
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

TSubclassOf<UCameraShakeBase> AMyCharacter::GetCamShake()	
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
	FGameplayTag FlyingTag = MyGameplayTags::Activate_FlyingAttack;
	FGameplayTagContainer FlyingTagContainer = FGameplayTagContainer(FlyingTag);
	FGameplayTag GroundTag = MyGameplayTags::Activate_GroundAttack;
	FGameplayTagContainer GroundTagContainer = FGameplayTagContainer(GroundTag);
	AbilitySystem->SetLooseGameplayTagCount(MyGameplayTags::Status_Airborne, GetMovementComponent()->IsFalling() ? 1 : 0);
	if (!GetMovementComponent()->IsFalling())	// I'm on ground
	{
		AbilitySystem->BlockAbilitiesWithTags(FlyingTagContainer);
		AbilitySystem->UnBlockAbilitiesWithTags(GroundTagContainer);
		AbilitySystem->CancelAbilities(&FlyingTagContainer);
		GetCapsuleComponent()->SetGenerateOverlapEvents(true);
		GetMesh()->SetGenerateOverlapEvents(false);
	}
	else										// I'm falling
	{
		AbilitySystem->BlockAbilitiesWithTags(GroundTagContainer);
		AbilitySystem->UnBlockAbilitiesWithTags(FlyingTagContainer);
		AbilitySystem->CancelAbilities(&GroundTagContainer);
		GetCapsuleComponent()->SetGenerateOverlapEvents(false);
		GetMesh()->SetGenerateOverlapEvents(true);
		LastGroundLocation = GetActorLocation();
	}
	RefreshPawnCollision();
}

uint8 AMyCharacter::GetTeam()
{
	return Team;
}

/// <summary>
/// New Item was just picked up. Apply its effects, learn its skills.
/// If it's not a consumable, then add it to the inventory.
/// </summary>
/// <param name="NewItem">The Item we just picked up</param>
void AMyCharacter::AddItemToInventory(UItemDataAsset* NewItem)
{
	ApplyOneItemEffect(NewItem);
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

/// <summary>
/// Applies all effects from all items in the inventory. Used at begin play to update the just created char with all the items.
/// </summary>
void AMyCharacter::ApplyAllItemEffects()
{
	for (auto &&Item : *Inventory)
	{
		ApplyOneItemEffect(Item);
	}
}

/// <summary>
/// Apply this one item to the character. Apply all effects from the Item and learn all skills from the item.
/// </summary>
/// <param name="NewItem">The Item to apply</param>
void AMyCharacter::ApplyOneItemEffect(UItemDataAsset* NewItem)
{
	UMyBlueprintFunctionLibrary::ApplyAllEffectContainersToChar(this, NewItem->EffectsToApply, NewItem);
	ULearnItemDataAsset* LearnData = Cast<ULearnItemDataAsset>(NewItem);
	if (LearnData) LearnAbilities(LearnData->AbilitiesToLearn);
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
	FGameplayTag NoControlTag = MyGameplayTags::Data_NoControl;
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
	FGameplayTag HitStunTag = MyGameplayTags::Status_HitStun;
	FGameplayTag NoControlTag = MyGameplayTags::Status_NoControl;
    if(AbilitySystem->HasMatchingGameplayTag(HitStunTag) || AbilitySystem->HasMatchingGameplayTag(NoControlTag))
	{
		return false;
	}
	return bHasControl;
}

bool AMyCharacter::HasMoveControl()
{
	FGameplayTag HitStunTag = MyGameplayTags::Status_HitStun;
	FGameplayTag NoControlTag = MyGameplayTags::Status_NoControl;
	FGameplayTag NoMoveTag = MyGameplayTags::Status_NoMove;
	if (AbilitySystem->HasMatchingGameplayTag(HitStunTag) || AbilitySystem->HasMatchingGameplayTag(NoControlTag) || AbilitySystem->HasMatchingGameplayTag(NoMoveTag))
	{
		return false;
	}
	return true;
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
			MyCont->UpdateHUDAbilityKey((EInput)101, false);
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
			MyCont->UpdateHUDAbilityKey((EInput)101, true);
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

/// <summary>
/// Returns if I am immune to projectiles. Checks the Actor for the tag status.immune.proj.
/// </summary>
/// <returns>True if immune to projectiles, false otherwise.</returns>
bool AMyCharacter::IsProjectileImmune()
{
	FGameplayTag ImmuneTag = MyGameplayTags::Status_ImmuneProjectile;
	if (AbilitySystem->HasMatchingGameplayTag(ImmuneTag))
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
