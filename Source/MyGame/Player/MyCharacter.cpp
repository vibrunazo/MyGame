// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MyCharacter.h"
#include "../MyGameplayTags.h"
#include "AssTags.h"
#include "AssTestProbe.h"
#include "EnemyCharBase.h"
#include "MyPlayerController.h"
#include "MyAIController.h"
#include "../MyGameInstance.h"
#include "../Abilities/LootComponent.h"
#include "../Level/FloorBuilder.h"
#include "../Level/RunSubsystem.h"
#include "../Props/ItemDataAsset.h"
#include "../Props/LearnItemDataAsset.h"
#include "../Props/Pickup.h"
#include "../UI/MyHealthBar.h"
#include "AssCombatComponent.h"
#include "AssAbilitySystemComponent.h"
#include "../Level/TileTemplate.h"
#include "../Props/Spawner.h"
#include "Kismet/KismetSystemLibrary.h"

//#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Components/InputComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Animation/AnimMontage.h"
#include "AssAbility.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
// #include "../Abilities/MyAttributeSet.h"
#include "MyAnimInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
//#include "Kismet/GameplayStatics.h"
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
	

	// Our ability system component.
	AbilitySystem = CreateDefaultSubobject<UAssAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSetBase = CreateDefaultSubobject<UMyAttributeSet>(TEXT("AttributeSetBase"));
	LootComponent = CreateDefaultSubobject<ULootComponent>(TEXT("Loot Component"));
	Combat = CreateDefaultSubobject<UAssCombatComponent>(TEXT("Combat"));


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

	HealthBarComp->SetRelativeLocation(FVector(0.0f, 0.0f, -140.0f));
	HealthBarComp->SetWidgetSpace(EWidgetSpace::Screen);

	// The anim class and the AI controller are set on the character Blueprints, not here: a path in native code
	// isn't a reference the cooker can see, so the packaged game would lack the asset
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

UAbilitySystemComponent* AMyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
};

void AMyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	Combat->BeginCombat();
	if(!AbilitySystem) return;
	AbilitySystem->InitAbilityActorInfo(this, this);
	Combat->BindAbilitySystem(AbilitySystem);
	// on later floors the player starts with the moves carried from the last one, learned books included
	if (IsPlayerControlled() && GetMyGameInstance() && GetMyGameInstance()->CarriedAbilities.Num() > 0)
	{
		Abilities = GetMyGameInstance()->CarriedAbilities;
	}
	for (const FAssAbilityGrant& Grant : Abilities)
	{
		GiveGrant(Grant);
	}
	if (AMyPlayerController* MyCont = GetController<AMyPlayerController>()) MyCont->RefreshHUDAbilities(this);
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
	Combat->RefreshPawnCollision();
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
	BodyMaterials.Reset();
	for (int32 Slot = 0; Slot < GetMesh()->GetNumMaterials(); ++Slot)
	{
		if (UMaterialInstanceDynamic* Material = GetMesh()->CreateDynamicMaterialInstance(Slot)) BodyMaterials.Add(Material);
	}
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


void AMyCharacter::GiveGrant(const FAssAbilityGrant& Grant)
{
	// the same template can be learned again (another item with it); one grant is enough
	if (HasAuthority() && !Grant.Template.IsNull() && !FindGrant(Grant.Template).IsValid())
	{
		AbilitySystem->GiveGrant(Grant);
	}
}

FGameplayAbilitySpecHandle AMyCharacter::FindGrant(const TSoftObjectPtr<UAssAbilityTemplate>& Template) const
{
	if (!AbilitySystem || Template.IsNull()) return FGameplayAbilitySpecHandle();
	for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		const UAssAbilityTemplate* Granted = UAssAbilitySystemComponent::GetGrantTemplate(Spec);
		if (Granted && FSoftObjectPath(Granted) == Template.ToSoftObjectPath()) return Spec.Handle;
	}
	return FGameplayAbilitySpecHandle();
}

void AMyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// refresh the ASC's actor info (player controller) now that we have a controller
	if (AbilitySystem) AbilitySystem->InitAbilityActorInfo(this, this);
	// a plain engine controller means the Blueprint's AI controller is missing (unset, or not in a packaged build)
	if (NewController && !NewController->IsA<APlayerController>() && !NewController->IsA<AMyAIController>())
	{
		UE_LOG(LogTemp, Error, TEXT("%s has no game AI: possessed by %s"), *GetName(), *NewController->GetClass()->GetName());
	}
}

void AMyCharacter::LearnAbility(FAssAbilityGrant Ability)
{
	if (Ability.Template.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("trying to learn ability but the grant has no template"));
		return;
	}
	FindAndRemoveOverlappingAbilities(Ability);
	UE_LOG(LogTemp, Warning, TEXT("Learning Ability: %s"), *Ability.Template.GetAssetName());
	Abilities.Add(Ability);
	GiveGrant(Ability);
	AMyPlayerController* MyCont = GetController<AMyPlayerController>();
	if (!MyCont) return;
	MyCont->UpdateHUDAbility(Ability, true);
		//UMyHUDWidget* HUDWidgetRef
}

void AMyCharacter::LearnAbilities(TArray<FAssAbilityGrant> NewAbilities)
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
void AMyCharacter::FindAndRemoveOverlappingAbilities(FAssAbilityGrant AbilityToCompare)
{
	auto Same = [](const FAssAbilityGrant& A, const FAssAbilityGrant& B) { return A.Template == B.Template && A.SlotKey == B.SlotKey; };
	TArray<FAssAbilityGrant> AbilitiesToRemove;
	for (const FAssAbilityGrant& LearnedAbility : Abilities)
	{
		// same slot, and both air moves or both ground moves (an air punch doesn't replace the ground punch)
		if (LearnedAbility.SlotKey == AbilityToCompare.SlotKey && IsAirAbility(LearnedAbility.Template) == IsAirAbility(AbilityToCompare.Template))
		{
			AbilitiesToRemove.Add(LearnedAbility);
		}
	}
	for (const FAssAbilityGrant& OldAbility : AbilitiesToRemove)
	{
		UE_LOG(LogTemp, Warning, TEXT("Removing Overlapping Ability: %s"), *OldAbility.Template.GetAssetName());
		Abilities.RemoveAll([&](const FAssAbilityGrant& A) { return Same(A, OldAbility); });
		// the replaced ability must leave the ASC too, unless it's being re-learned or another slot uses the same template
		const bool bStillUsed = OldAbility.Template == AbilityToCompare.Template
			|| Abilities.ContainsByPredicate([&](const FAssAbilityGrant& A) { return A.Template == OldAbility.Template; });
		const FGameplayAbilitySpecHandle Handle = bStillUsed ? FGameplayAbilitySpecHandle() : FindGrant(OldAbility.Template);
		if (Handle.IsValid()) AbilitySystem->ClearGrant(Handle);
	}
}

#if WITH_EDITOR
void AMyCharacter::SetAttributeBaseForTest(FGameplayAttribute Attribute, float Value)
{
	if (AbilitySystem && Attribute.IsValid()) AbilitySystem->SetNumericAttributeBase(Attribute, Value);
	UpdateHealthBar();
}

TArray<FString> AMyCharacter::GetTagsForTest() const
{
	TArray<FString> Out;
	if (!AbilitySystem) return Out;
	FGameplayTagContainer Owned;
	AbilitySystem->GetOwnedGameplayTags(Owned);
	for (const FGameplayTag& Tag : Owned) Out.Add(Tag.ToString());
	return Out;
}

bool AMyCharacter::ActivateTemplateForTest(UAssAbilityTemplate* Template)
{
	const FGameplayAbilitySpecHandle Grant = FindGrant(Template);
	return AbilitySystem && Grant.IsValid() && AbilitySystem->TryActivateGrant(Grant);
}

TArray<FString> AMyCharacter::GetActiveAbilitiesForTest() const
{
	TArray<FString> Out;
	if (!AbilitySystem) return Out;
	for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		const UAssAbilityTemplate* Template = UAssAbilitySystemComponent::GetGrantTemplate(Spec);
		if (Spec.IsActive() && Template) Out.Add(Template->GetName());
	}
	return Out;
}
#endif

bool AMyCharacter::IsAirAbility(const TSoftObjectPtr<UAssAbilityTemplate>& Template)
{
	const UAssAbilityTemplate* Loaded = Template.LoadSynchronous();
	return Loaded && Loaded->Definition.Tags.Identity.HasTagExact(MyGameplayTags::Activate_FlyingAttack);
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
	TArray<FAssAbilityGrant> AbilitiesThatCanActivate;
	// first add all abilities that can activate from this input (ie punch) to a list
	for (const FAssAbilityGrant& Ability : Abilities)
	{
		// ground moves are blocked while airborne and air moves on the ground (OnMovementModeChanged), plus each
		// definition's own tags (its identity not blocked, its required and blocked tags): the fallback below only
		// considers moves whose tags allow them right now, as the class's tag requirements did
		const UAssAbilityTemplate* Template = Ability.Template.LoadSynchronous();
		if (Ability.SlotKey < 0 || Ability.SlotKey > MAX_uint8 || !InputsToCheck.Contains((uint8)Ability.SlotKey) || !Template) continue;
		const FAssAbilityTags& GrantTags = Template->Definition.Tags;
		if (!AbilitySystem->AreAbilityTagsBlocked(GrantTags.Identity) && AbilitySystem->HasAllMatchingGameplayTags(GrantTags.Required)
			&& !AbilitySystem->HasAnyMatchingGameplayTags(GrantTags.Blocked))
		{
			AbilitiesThatCanActivate.Add(Ability);
		}
	}
	// then, activate only the highest priority ability with that button
	// ie. if pressing super mod + punch, will activate only the super punch, not regular punch
	if (AbilitiesThatCanActivate.Num() == 0) return;
	int32 BestAbilityScore = 0;
	FAssAbilityGrant BestAbility;
	for (const FAssAbilityGrant& Ability : AbilitiesThatCanActivate)
	{
		if (Ability.SlotKey >= BestAbilityScore)
		{
			BestAbility = Ability;
			BestAbilityScore = Ability.SlotKey;
		}
	}
	AbilitySystem->TryActivateGrant(FindGrant(BestAbility.Template));
	if (MyCont) MyCont->UpdateHUDAbilityKey(UMyBlueprintFunctionLibrary::GetGrantInput(BestAbility), true);

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

FActiveGameplayEffectHandle AMyCharacter::OnGetHitByEffect(FGameplayEffectSpecHandle NewEffect)
{
	if (!ensure(NewEffect.Data) || !ensure(AbilitySystem)) return FActiveGameplayEffectHandle();
	// hitstun, knockback, launch and camera shake happen through the effects' Hit Reaction components
	FActiveGameplayEffectHandle ActiveEffect = AbilitySystem->ApplyGameplayEffectSpecToSelf(*NewEffect.Data.Get());
	UpdateHealthBar();
	return ActiveEffect;
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

void AMyCharacter::OnDie()
{
	// UE_LOG(LogTemp, Warning, TEXT("I died"));
	if (IsAlive() && AttributeSetBase)
	{
		AttributeSetBase->SetHealth(0.f);
	}
	UWorld* World = GetWorld();
	// FTimerManager TM = FTimerManager::FTimerManager;
	// the body's launch and the delayed death are the authority's (Ass Step8.md 4.5), kept as world timers: one fires
	// once world time is past death + its delay, the rule the plugin's hit launch follows
	if (HasAuthority())
	{
		FTimerHandle Handle;
		FTimerHandle Handle2;
		GetWorldTimerManager().SetTimer(Handle, this, &AMyCharacter::OnDelayedDeath, 5.0f, false);
		GetWorldTimerManager().SetTimer(Handle2, this, &AMyCharacter::OnDelayedLaunch2, .05f, false);
	}
	Combat->SetControlEnabled(false);
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
	Combat->RefreshPawnCollision();
	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	UMyAnimInstance* MyAnim = Cast<UMyAnimInstance>(Anim);

	if (DeathSound) ASS_TEST_PROBE(this, "sound", nullptr, DeathSound->GetPathName());
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
	// the airborne tag, the ground and air ability blocks, which of capsule and mesh generate overlaps
	Combat->OnMovementModeChanged();
	if (GetMovementComponent()->IsFalling()) LastGroundLocation = GetActorLocation();
	Combat->RefreshPawnCollision();
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

UPrimitiveComponent* AMyCharacter::GetTargetDetection()
{
	return TargetDetection;
}

UNiagaraSystem* AMyCharacter::GetSlashEffect(EAssSlashKind Kind)
{
	return Kind == EAssSlashKind::Kick ? KickSlash : PunchSlash;
}

bool AMyCharacter::Summon(const FAssSummon& /*Summon*/)
{
	// GA_Summon's graph, in its order: the spawners (WorldStatic) overlapping the box of this tile, the one farthest from
	// the target strictly beyond 0 (so none with no target), then its SpawnActors (which counts the summon on its tile)
	AFloorBuilder* Builder = URunSubsystem::GetFloorBuilder(this);
	const FVector Center = Builder ? Builder->GetTileCenter(GetActorLocation()) : FVector::ZeroVector;
	const FVector Extent = GetDefault<UTileTemplate>()->RoomSize / 2.f;
	TArray<AActor*> Found;
	UKismetSystemLibrary::BoxOverlapActors(this, Center, Extent, { UEngineTypes::ConvertToObjectType(ECC_WorldStatic) },
		ASpawner::StaticClass(), {}, Found);
	AActor* Target = GetTargetEnemy();
	float BestDistance = 0.f;
	ASpawner* Best = nullptr;
	for (AActor* Actor : Found)
	{
		const float Distance = Target ? Actor->GetDistanceTo(Target) : 0.f;
		if (Distance > BestDistance)
		{
			BestDistance = Distance;
			Best = Cast<ASpawner>(Actor);
		}
	}
	if (!Best) return false;
	Best->SpawnActors();
	return true;
}

bool AMyCharacter::GetStrikeSound(EAssSoundKind Kind, USoundBase*& OutSound)
{
	switch (Kind)
	{
	case EAssSoundKind::PunchWeak:
		OutSound = PunchWeakWoosh;
		return true;
	case EAssSoundKind::PunchStrong:
		OutSound = PunchStrongWoosh;
		return true;
	default:
		return false;
	}
}

#if WITH_EDITOR
TFunction<void(AMyCharacter* Owner, AActor* Asker, bool bAdd, bool bAnswer)> AMyCharacter::AggroListProbe;
#endif

void AMyCharacter::AddToAggroList(AActor* NewActor)
{
	if (NewActor)
	{
		AggroList.Add(NewActor);
#if WITH_EDITOR
		if (AggroListProbe) AggroListProbe(this, NewActor, true, true);
#endif
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
	const bool bAnswer = AnswerAggroList(NewActor);
#if WITH_EDITOR
	if (AggroListProbe) AggroListProbe(this, NewActor, false, bAnswer);
#endif
	return bAnswer;
}

bool AMyCharacter::AnswerAggroList(AActor* NewActor)
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

FTransform AMyCharacter::GetProjectileSpawn()
{
	return SpawnArrow->GetComponentTransform();
	// UE_LOG(LogTemp, Warning, TEXT("Char casting projectile"));
}

void AMyCharacter::PlayHitCameraShake(float Strength)
{
	if (GetMyGameInstance()) GetMyGameInstance()->DoCamShake(Strength);
}

void AMyCharacter::PlayHitStunMontage()
{
	// GetHitMontage is fetched from the anim instance the first time this character is damaged (OnDamaged): a stun
	// that lands before any damage plays none
	PlayAnimMontage(GetHitMontage);
}

bool AMyCharacter::HasControl()
{
	return Combat->HasControl();
}

bool AMyCharacter::HasMoveControl()
{
	return Combat->HasMoveControl();
}


void AMyCharacter::SetAggroTarget(APawn* NewTarget)
{
	IAssCombatant* NewChar = Cast<IAssCombatant>(NewTarget);
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

	AFloorBuilder* Builder = URunSubsystem::GetFloorBuilder(this);
	if (!Builder) return;
	Builder->AggroTile(NewTarget, GetActorLocation());
	if (AggroSound) ASS_TEST_PROBE(this, "sound", nullptr, AggroSound->GetPathName());
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
/// Returns if I am immune to projectiles. Checks the Actor for the tag Ass.Status.ImmuneProjectile.
/// </summary>
/// <returns>True if immune to projectiles, false otherwise.</returns>
bool AMyCharacter::IsProjectileImmune()
{
	FGameplayTag ImmuneTag = AssTags::Status_ImmuneProjectile;
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
	AFloorBuilder* Builder = URunSubsystem::GetFloorBuilder(this);
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

void AMyCharacter::SetBodyMaterialsVector(FName Parameter, const FLinearColor& Value)
{
	for (UMaterialInstanceDynamic* Material : BodyMaterials) if (Material) Material->SetVectorParameterValue(Parameter, Value);
}

void AMyCharacter::SetBodyMaterialsScalar(FName Parameter, float Value)
{
	for (UMaterialInstanceDynamic* Material : BodyMaterials) if (Material) Material->SetScalarParameterValue(Parameter, Value);
}

void AMyCharacter::SetBodyColor(FLinearColor NewColor)
{
	SetBodyMaterialsVector(FName("BodyColor"), NewColor);
}

void AMyCharacter::ResetBodyColor()
{
	SetBodyMaterialsVector(FName("BodyColor"), BodyColor);
}

void AMyCharacter::SetGlow(float NewGlow)
{
	SetBodyMaterialsScalar(FName("GlowAlpha"), NewGlow);
}

void AMyCharacter::ResetGlow()
{
	SetBodyMaterialsScalar(FName("GlowAlpha"), 0.f);
}
