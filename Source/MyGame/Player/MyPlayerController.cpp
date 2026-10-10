// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "../MyGameplayTags.h"
#include "MyCharacter.h"
#include "MyDefaultPawn.h"
#include "../UI/MyUserWidget.h"
#include "../UI/MyHUDWidget.h"
#include "../MyGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (UEnhancedInputLocalPlayerSubsystem* Input = GetEnhancedInputSubsystem())
    {
        if (DefaultMappingContext) Input->AddMappingContext(DefaultMappingContext, 0);
    }
    ShowHUD();
    ShowLevelIntro();
}

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
    if (MoveAction)
    {
        Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyPlayerController::OnMove);
        Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMyPlayerController::OnMove);
    }
    for (const FAbilityInputBinding& Binding : AbilityInputs)
    {
        if (!Binding.Action) continue;
        Input->BindAction(Binding.Action, ETriggerEvent::Started, this, &AMyPlayerController::OnAbilityInput, Binding.Slot, true);
        Input->BindAction(Binding.Action, ETriggerEvent::Completed, this, &AMyPlayerController::OnAbilityInput, Binding.Slot, false);
    }
    if (SuperModAction)
    {
        Input->BindAction(SuperModAction, ETriggerEvent::Started, this, &AMyPlayerController::SetSuperMod, true);
        Input->BindAction(SuperModAction, ETriggerEvent::Completed, this, &AMyPlayerController::SetSuperMod, false);
    }
    if (UltraModAction)
    {
        Input->BindAction(UltraModAction, ETriggerEvent::Started, this, &AMyPlayerController::SetUltraMod, true);
        Input->BindAction(UltraModAction, ETriggerEvent::Completed, this, &AMyPlayerController::SetUltraMod, false);
    }
    // IA_Pause triggers while paused, so the same button closes the menu
    if (PauseAction) Input->BindAction(PauseAction, ETriggerEvent::Started, this, &AMyPlayerController::OnPausePressed);
    if (ShowFPSAction) Input->BindAction(ShowFPSAction, ETriggerEvent::Started, this, &AMyPlayerController::OnShowFPS);
}

UEnhancedInputLocalPlayerSubsystem* AMyPlayerController::GetEnhancedInputSubsystem() const
{
    return ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
}

#if WITH_EDITOR
void AMyPlayerController::InjectTestInput(UInputAction* Action, FVector2D Value, bool bHeld)
{
    UEnhancedInputLocalPlayerSubsystem* Input = GetEnhancedInputSubsystem();
    if (!Input || !Action) return;
    if (bHeld) Input->StartContinuousInputInjectionForAction(Action, FInputActionValue(Action->ValueType, FVector(Value, 0.)), {}, {});
    else Input->StopContinuousInputInjectionForAction(Action);
}
#endif

void AMyPlayerController::OnMove(const FInputActionValue& Value)
{
    if (AMyCharacter* MyChar = GetPawn<AMyCharacter>()) MyChar->SetMoveInput(Value.Get<FVector2D>());
}

void AMyPlayerController::OnAbilityInput(EInput Slot, bool bPressed)
{
    if (Slot == EInput::Jump)
    {
        if (bPressed) Jump();
        else StopJump();
    }
    SetAbilityKeyDown(Slot, bPressed);
}

void AMyPlayerController::OnShowFPS()
{
    ConsoleCommand(TEXT("stat fps"));
}

void AMyPlayerController::Jump()
{
    UpdateHUDAbilityKey(EInput::Jump, true);
    AMyCharacter* MyChar = GetPawn<AMyCharacter>();
    if (MyChar) MyChar->Jump();
}

void AMyPlayerController::StopJump()
{
    UpdateHUDAbilityKey(EInput::Jump, false);
    AMyCharacter* MyChar = GetPawn<AMyCharacter>();
    if (MyChar) MyChar->StopJumping();
}

void AMyPlayerController::OnPausePressed()
{
    UE_LOG(LogTemp, Warning, TEXT("Pause menu"));
    if (!bIsPaused)
    {
        if (!PauseWidget) return;
        PauseWidgetRef = CreateWidget<UMyUserWidget>(this, PauseWidget);
        PauseWidgetRef->AddToViewport();
        SetInputMode(FInputModeGameAndUI());
        bShowMouseCursor = true;
        if (!GetWorld()) return;
        UGameplayStatics::SetGamePaused(GetWorld(), true);
        bIsPaused = true;
    }
    else
    {
        if (PauseWidgetRef) PauseWidgetRef->BPOnEnd();
        // if (PauseWidgetRef) PauseWidgetRef->RemoveFromParent();
        UGameplayStatics::SetGamePaused(GetWorld(), false);
        bIsPaused = false;

    }
}

void AMyPlayerController::ShowHUD()
{
    UE_LOG(LogTemp, Warning, TEXT("showing HUD"));
    if (!HUDWidget) return;
    HUDWidgetRef = CreateWidget<UMyHUDWidget>(this, HUDWidget);
    HUDWidgetRef->AddToViewport();
}

void AMyPlayerController::ShowLevelIntro()
{
    UE_LOG(LogTemp, Warning, TEXT("level Intro widget"));
    if (!IntroWidget) return;
    IntroWidgetRef = CreateWidget<UMyUserWidget>(this, IntroWidget);
    IntroWidgetRef->AddToViewport();
}

void AMyPlayerController::UpdateHUD(AMyCharacter* Char)
{
    UMyAttributeSet* Attr = Char->GetAttributes();
    if (!Attr) return;
    if (!HUDWidgetRef) return;
    HUDWidgetRef->BPUpdateHUD(Char);
    HUDWidgetRef->BPUpdateHealth(Attr->GetHealth());
    HUDWidgetRef->BPUpdateMana(Attr->GetMana());
}

float AMyPlayerController::GetHUDHealth()
{
    if (!HUDWidgetRef) return 0.0f;
    return HUDWidgetRef->GetHealth();
}


void AMyPlayerController::SetAbilityKeyDown(EInput Index, bool IsKeyDown)
{
    if ((uint8)Index < AbilityKeyStates.Num()) AbilityKeyStates[(uint8)Index] = IsKeyDown;
    AMyCharacter* MyChar = GetPawn<AMyCharacter>();
    if (MyChar) MyChar->SetAbilityKeyDown((uint8)Index, IsKeyDown);
    if (!IsKeyDown)
    {
        UpdateHUDAbilityKey(Index, IsKeyDown);
        // abilities waiting for their button to be let go (charge moves) listen for input.release.<slot>
        static const FGameplayTag ReleaseTags[] = { MyGameplayTags::Input_Release_0, MyGameplayTags::Input_Release_1,
            MyGameplayTags::Input_Release_2, MyGameplayTags::Input_Release_3 };
        if (MyChar && (uint8)Index < UE_ARRAY_COUNT(ReleaseTags))
        {
            UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MyChar, ReleaseTags[(uint8)Index], FGameplayEventData());
        }
    }
}

bool AMyPlayerController::IsAbilityKeyDown(uint8 Index)
{
    if (Index < AbilityKeyStates.Num()) return AbilityKeyStates[Index];
    return false;
}

/// <summary>
/// Tell the HUD to show this ability key as pressed.
/// </summary>
/// <param name="Index">What ability?</param>
/// <param name="IsKeyDown">Is it pressed?</param>
/// <param name="Duration">For how long should it be pressed? If zero, stays pressed until told otherwise. If > 0 then the action button sets a timer to automatically unpress after Duration seconds.</param>
void AMyPlayerController::UpdateHUDAbilityKey(EInput Index, bool IsKeyDown, float Duration)
{
    uint8 HUDIndex = (uint8)Index;
    if (IsKeyDown)  // if I'm activating the skill
    {               // then activate only the one with the specific mod
        //if ((uint8)Index < 100) HUDIndex += GetModValue();
        if (HUDWidgetRef) HUDWidgetRef->BPUpdateAbilityKey((uint8)HUDIndex, IsKeyDown, Duration);
    }
    else            // if I'm deactivating
    {               // then deactivate all mods
        if (HUDWidgetRef) HUDWidgetRef->BPUpdateAbilityKey((uint8)Index, IsKeyDown, Duration);
        if (HUDWidgetRef) HUDWidgetRef->BPUpdateAbilityKey((uint8)Index + 10, IsKeyDown, Duration);
        if (HUDWidgetRef) HUDWidgetRef->BPUpdateAbilityKey((uint8)Index + 20, IsKeyDown, Duration);
    }
}

void AMyPlayerController::ShowAbilityCooldown(uint8 Index, float Cooldown)
{
    if (HUDWidgetRef) HUDWidgetRef->BPUpdateCooldown(Index, Cooldown);
}

void AMyPlayerController::UpdateHUDAbility(FAbilityStruct Ability, bool NewState)
{
    if (HUDWidgetRef)
    {
        HUDWidgetRef->BPSetupAbility(Ability);
        HUDWidgetRef->BPUpdateActionSlot(Ability.Input, NewState);
    }
}

void AMyPlayerController::SetSuperMod(bool NewState)
{
    bSuperMod = NewState;
}

bool AMyPlayerController::GetSuperMod()
{
    return bSuperMod;
}

void AMyPlayerController::SetUltraMod(bool NewState)
{
    bUltraMod = NewState;
}

bool AMyPlayerController::GetUltraMod()
{
    return bUltraMod;
}

/// <summary>
/// Returns how much to add to the input enum based on mods. For example, 1 is Kick, 11 is SuperKick, 21 is UltraKick.
/// </summary>
/// <returns>0 if no mod is pressed, 10 if Super mod is pressed, 20 is Ultra mod is pressed</returns>
uint8 AMyPlayerController::GetModValue()
{
    if (GetUltraMod()) return 20;
    if (GetSuperMod()) return 10;
    return 0;
}

void AMyPlayerController::OnCharDies(AMyCharacter* CharRef)
{
    if (bIsLevelOver) return;
    bIsLevelOver = true;
    FVector Loc = CharRef->GetActorLocation();
    FActorSpawnParameters params;
	AMyDefaultPawn* MyPawn = GetWorld()->SpawnActor<AMyDefaultPawn>(AMyDefaultPawn::StaticClass(), Loc, FRotator::ZeroRotator, params);
    DefaultPawnRef = MyPawn;
    SetViewTargetWithBlend(DefaultPawnRef, 3.0f);
    ShowGameOver();
    if (!GetGameInstance()) return;
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
	if (GI)
	{
		GI->OnGameOver();
	}
}

void AMyPlayerController::OnDelayedCharDies(AMyCharacter* CharRef)
{
    Possess(DefaultPawnRef);

}

void AMyPlayerController::ShowGameOver()
{
    if (!GameOverWidget) return;
    GameOverWidgetRef = CreateWidget<UMyUserWidget>(this, GameOverWidget);
    GameOverWidgetRef->AddToViewport();
    SetInputMode(FInputModeUIOnly());
    bShowMouseCursor = true;
}

void AMyPlayerController::OnLevelWin(AMyCharacter* CharRef)
{
    if (bIsLevelOver) return;
    bIsLevelOver = true;
    CharRef->bHasControl = false;
	CharRef->DisableInput(nullptr);
    FVector Loc = CharRef->GetActorLocation();
    FActorSpawnParameters params;
	AMyDefaultPawn* MyPawn = GetWorld()->SpawnActor<AMyDefaultPawn>(AMyDefaultPawn::StaticClass(), Loc, FRotator::ZeroRotator, params);
    DefaultPawnRef = MyPawn;
    SetViewTargetWithBlend(DefaultPawnRef, 3.0f);
    ShowLevelCleared();
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
	if (GI)
	{
		GI->StoreCharStats();
	}
}

void AMyPlayerController::OnDelayedLevelWin(AMyCharacter* CharRef)
{
    Possess(DefaultPawnRef);

}

void AMyPlayerController::ShowLevelCleared()
{
    if (!LevelClearedWidget) return;
    LevelClearedWidgetRef = CreateWidget<UMyUserWidget>(this, LevelClearedWidget);
    LevelClearedWidgetRef->AddToViewport();
    // SetInputMode(FInputModeUIOnly());
    // bShowMouseCursor = true;
}