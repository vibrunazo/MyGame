// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAIController.h"
#include "MyCharacter.h"
#include "MyPatternAsset.h"
#include "MyPatternDriver.h"
#include "MyPatternStepper.h"
#include "AssMoverComponent.h"
#include "../MyGameplayTags.h"
#include "AssAbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

AMyAIController::AMyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	// The values every enemy's old PawnSensing used: 700 units, 90 degrees to each side, players only (filtered below)
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 700.f;
	SightConfig->LoseSightRadius = 800.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	Perception->ConfigureSense(*SightConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*Perception);

	PatternDriver = CreateDefaultSubobject<UMyPatternDriver>(TEXT("PatternDriver"));
	PatternMover = CreateDefaultSubobject<UAssMoverComponent>(TEXT("PatternMover"));
	PatternMover->bChaseArrives = false;
	PatternMover->bFailWhenPinned = false;
	PatternMover->bAcceptPartialPaths = true;
}

bool AMyAIController::RunBehaviorTree(UBehaviorTree* BTAsset)
{
	if (UsesPattern()) return false;
	return Super::RunBehaviorTree(BTAsset);
}

ACharacter* AMyAIController::GetPatternTarget() const
{
	return PatternDriver ? PatternDriver->GetPatternTarget() : nullptr;
}

void AMyAIController::SetPatternTarget(AActor* Target)
{
	if (PatternDriver) PatternDriver->SetTarget(Cast<ACharacter>(Target));
}

void AMyAIController::StepPattern()
{
	if (!UsesPattern() || !PatternDriver || bPatternFrozen) return;
	EnsurePatternStarted();
	PatternDriver->TickPattern();
}

void AMyAIController::BeginPlay()
{
	Super::BeginPlay();
	if (UAIPerceptionComponent* Perception = GetAIPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &AMyAIController::OnTargetPerceived);
	}
}

void AMyAIController::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	APawn* SeenPawn = Cast<APawn>(Actor);
	if (!Stimulus.WasSuccessfullySensed() || !SeenPawn || !SeenPawn->IsPlayerControlled()) return;
	if (AMyCharacter* Me = GetPawn<AMyCharacter>()) Me->OnPawnSeen(SeenPawn);
}

void AMyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RespondedEvents.Reset();
	for (const FAIEventResponse& Response : EventResponses) RespondedEvents.AddTag(Response.Event);
	PawnAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InPawn);
	if (PawnAbilitySystem.IsValid() && !RespondedEvents.IsEmpty())
	{
		PawnEventHandle = PawnAbilitySystem->AddGameplayEventTagContainerDelegate(RespondedEvents,
			FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &AMyAIController::OnPawnGameplayEvent));
	}
	if (PawnAbilitySystem.IsValid())
	{
		BuffedRangeHandle = PawnAbilitySystem->RegisterGameplayTagEvent(MyGameplayTags::Status_BuffedRange, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &AMyAIController::OnBuffedRangeChanged);
	}
	// The pawn's movement ticks after this controller's path following (a move's request), as the engine already puts it
	// after the controller (AddPawnTickDependency), so a move's first and last frames turn the pawn the same way every
	// time; left unordered, which ticked first fell to the world's tick history, and a turn could land a frame apart from
	// one world to the next
	if (ACharacter* AsCharacter = Cast<ACharacter>(InPawn); AsCharacter && GetPathFollowingComponent())
	{
		AsCharacter->GetCharacterMovement()->PrimaryComponentTick.AddPrerequisite(GetPathFollowingComponent(), GetPathFollowingComponent()->PrimaryComponentTick);
	}
	// On a pattern: registered now, stepped in spawn order; it starts once the pawn has its abilities (its first step, a
	// health mark or a wake, whichever comes first), with no target (the boss's marks settle before it sees anyone)
	if (UsesPattern() && PatternDriver)
	{
		PatternSalt = AMyPatternStepper::Register(this);
	}
}

void AMyAIController::EnsurePatternStarted()
{
	if (!UsesPattern() || !PatternDriver || PatternDriver->IsRunning()) return;
	const APawn* Me = GetPawn();
	// The pawn's grants come in its BeginPlay, after possession: the pattern reads which slots are filled when it starts
	if (!Me || !Me->HasActorBegunPlay()) return;
	PatternDriver->StartPattern(Pattern->Definition, PatternSalt);
	PatternWokeFrame = GFrameCounter;
}

void AMyAIController::SetPatternRunning(bool bRunning)
{
	if (!UsesPattern() || !PatternDriver) return;
	if (!bRunning)
	{
		// Frozen as the tree's brain is stopped: it steps no more and stops moving, but keeps its state, and its health
		// marks still settle (the tree's event responses were the controller's, not the brain's)
		bPatternFrozen = true;
		StopMovement();
		return;
	}
	EnsurePatternStarted();
	bPatternFrozen = false;
	PatternWokeFrame = GFrameCounter;
}

void AMyAIController::UsePatternNow(UMyPatternAsset* Asset)
{
	if (!Asset)
	{
		if (PatternDriver) PatternDriver->StopPattern();
		bUsePattern = false;
		Pattern = nullptr;
		return;
	}
	// The tree's target carries over
	UBlackboardComponent* BB = GetBlackboardComponent();
	AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(TEXT("TargetChar"))) : nullptr;
	if (UBrainComponent* Brain = GetBrainComponent()) Brain->StopLogic(TEXT("pattern"));
	StopMovement();
	bUsePattern = true;
	Pattern = Asset;
	PatternSalt = AMyPatternStepper::Register(this);
	PatternDriver->StartPattern(Pattern->Definition, PatternSalt);
	bPatternFrozen = false;
	PatternWokeFrame = GFrameCounter;
	if (Target) SetPatternTarget(Target);
}

bool AMyAIController::IsPatternRunning() const
{
	return PatternDriver && PatternDriver->IsRunning() && !bPatternFrozen;
}

void AMyAIController::OnBuffedRangeChanged(const FGameplayTag Tag, int32 Count)
{
	SetAttackRangeEverywhere(Count > 0 ? BuffedAttackRange : UnbuffedAttackRange);
}

void AMyAIController::SetAttackRangeEverywhere(float Range)
{
	AttackRange = Range;
	if (UBlackboardComponent* BB = GetBlackboardComponent()) BB->SetValueAsFloat(AttackRangeKey, Range);
}

void AMyAIController::OnUnPossess()
{
	if (PatternDriver) PatternDriver->StopPattern();
	if (ACharacter* AsCharacter = GetPawn<ACharacter>(); AsCharacter && GetPathFollowingComponent())
	{
		AsCharacter->GetCharacterMovement()->PrimaryComponentTick.RemovePrerequisite(GetPathFollowingComponent(), GetPathFollowingComponent()->PrimaryComponentTick);
	}
	if (PawnAbilitySystem.IsValid() && PawnEventHandle.IsValid())
	{
		PawnAbilitySystem->RemoveGameplayEventTagContainerDelegate(RespondedEvents, PawnEventHandle);
	}
	if (PawnAbilitySystem.IsValid() && BuffedRangeHandle.IsValid())
	{
		PawnAbilitySystem->RegisterGameplayTagEvent(MyGameplayTags::Status_BuffedRange, EGameplayTagEventType::NewOrRemoved).Remove(BuffedRangeHandle);
	}
	PawnEventHandle.Reset();
	BuffedRangeHandle.Reset();
	PawnAbilitySystem.Reset();
	Super::OnUnPossess();
}

void AMyAIController::OnPawnGameplayEvent(FGameplayTag Event, const FGameplayEventData* Payload)
{
	// On a pattern, a health mark settles its phases inside the hit that crossed it (their entries are the moves)
	if (UsesPattern() && PatternDriver)
	{
		EnsurePatternStarted();
		PatternDriver->SettlePattern();
		return;
	}
	UAssAbilitySystemComponent* ASC = Cast<UAssAbilitySystemComponent>(PawnAbilitySystem.Get());
	const AMyCharacter* Me = GetPawn<AMyCharacter>();
	if (!ASC || !Me) return;
	for (const FAIEventResponse& Response : EventResponses)
	{
		if (!Response.Event.MatchesTagExact(Event)) continue;
		for (const TSoftObjectPtr<UAssAbilityTemplate>& Template : Response.Templates)
		{
			const FGameplayAbilitySpecHandle Grant = Me->FindGrant(Template);
			if (Grant.IsValid()) ASC->TryActivateGrant(Grant);
		}
	}
}
