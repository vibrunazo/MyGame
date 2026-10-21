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
#include "InputMappingContext.h"
#include "InputKeyEventArgs.h"
#include "../UI/ActionButtonWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/UObjectIterator.h"

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (UEnhancedInputLocalPlayerSubsystem* Input = GetEnhancedInputSubsystem())
    {
        if (DefaultMappingContext) Input->AddMappingContext(DefaultMappingContext, 0);
    }
    // the input mode lives on the viewport and outlives a level: the menu and the game over screen leave it on
    // UI only, so a game level takes it back (the main menu has its own controller, AMenuController)
    SetInputMode(FInputModeGameOnly());
    SetShowMouseCursor(false);
    // until the player presses something, a plugged-in gamepad is the one they'll use
    bUsingGamepad = FSlateApplication::IsInitialized() && FSlateApplication::Get().IsGamepadAttached();
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
    if (DashAction) Input->BindAction(DashAction, ETriggerEvent::Triggered, this, &AMyPlayerController::OnDash);
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

AMyCharacter* AMyPlayerController::GetControlledChar() const
{
    AMyCharacter* MyChar = GetPawn<AMyCharacter>();
    return MyChar && MyChar->bHasControl ? MyChar : nullptr;
}

void AMyPlayerController::OnMove(const FInputActionValue& Value)
{
    if (AMyCharacter* MyChar = GetControlledChar()) MyChar->SetMoveInput(Value.Get<FVector2D>());
}

void AMyPlayerController::OnDash(const FInputActionValue& Value)
{
    if (AMyCharacter* MyChar = GetControlledChar()) MyChar->Dash(Value.Get<FVector2D>());
}

void AMyPlayerController::OnAbilityInput(EInput Slot, bool bPressed)
{
    // releases still go through, so nothing stays held
    if (bPressed && !GetControlledChar()) return;
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
        SetInputMode(FInputModeGameOnly());
        SetShowMouseCursor(false);
        bIsPaused = false;

    }
}

void AMyPlayerController::ShowHUD()
{
    UE_LOG(LogTemp, Warning, TEXT("showing HUD"));
    if (!HUDWidget) return;
    HUDWidgetRef = CreateWidget<UMyHUDWidget>(this, HUDWidget);
    HUDWidgetRef->AddToViewport();
    RefreshHUDAbilities(GetPawn<AMyCharacter>());
}

void AMyPlayerController::RefreshHUDAbilities(AMyCharacter* Char)
{
    if (!Char || !HUDWidgetRef) return;
    for (const FAbilityStruct& Ability : Char->Abilities)
    {
        // no slot (dash), or an air move sharing its slot with the ground move the button shows
        if (Ability.Input == EInput::None || AMyCharacter::IsAirAbility(Ability.AbilityClass)) continue;
        UpdateHUDAbility(Ability, true);
    }
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
    // the run resets when the next one starts (UMyGameInstance::StartNewRun from the game over screen)
    ShowGameOver();
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

bool AMyPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
    // a pressed key or button, or a stick pushed past its dead zone (mouse movement doesn't count)
    const bool bPressed = Params.Event == IE_Pressed || (Params.Key.IsAnalog() && FMath::Abs(Params.AmountDepressed) > 0.3f);
    if (Params.IsGamepad() && bPressed) SetUsingGamepad(true);
    else if (Params.Event == IE_Pressed && (Params.Key.IsMouseButton() || !Params.Key.IsAnalog()) && !Params.Key.IsTouch()) SetUsingGamepad(false);
    return Super::InputKey(Params);
}

void AMyPlayerController::SetUsingGamepad(bool bGamepad)
{
    if (bUsingGamepad == bGamepad) return;
    bUsingGamepad = bGamepad;
    for (TObjectIterator<UActionButtonWidget> Button; Button; ++Button)
    {
        if (Button->GetOwningPlayer() == this) Button->ShowInputDevice(bGamepad);
    }
}

FText AMyPlayerController::GetKeyboardText(EInput Slot) const
{
    if (Slot == EInput::Run) return KeyboardKeysText(MoveAction);
    if (Slot == EInput::Dash)
    {
        const FText Keys = KeyboardKeysText(DashAction);
        return Keys.IsEmpty() ? Keys : FText::Format(NSLOCTEXT("MyPlayerController", "DoubleTap", "{0} \u00D72"), Keys);
    }
    // Super and Ultra slots are the plain slot plus 10 or 20, pressed with their modifier held
    const uint8 Value = (uint8)Slot;
    if (Value >= 30) return FText::GetEmpty();
    const EInput Plain = (EInput)(Value % 10);
    const FAbilityInputBinding* Binding = AbilityInputs.FindByPredicate([Plain](const FAbilityInputBinding& B) { return B.Slot == Plain; });
    const FText Keys = Binding ? KeyboardKeysText(Binding->Action) : FText::GetEmpty();
    if (Keys.IsEmpty() || Value < 10) return Keys;
    const FText Modifier = KeyboardKeysText(Value >= 20 ? UltraModAction : SuperModAction);
    return Modifier.IsEmpty() ? FText::GetEmpty() : FText::Format(NSLOCTEXT("MyPlayerController", "ModifiedKey", "{0} + {1}"), Modifier, Keys);
}

FText AMyPlayerController::KeyboardKeysText(const UInputAction* Action) const
{
    if (!Action || !DefaultMappingContext) return FText::GetEmpty();
    TArray<FKey> Keys;
    for (const FEnhancedActionKeyMapping& Mapping : DefaultMappingContext->GetMappings())
    {
        if (Mapping.Action == Action && Mapping.Key.IsValid() && !Mapping.Key.IsGamepadKey() && !Keys.Contains(Mapping.Key)) Keys.Add(Mapping.Key);
    }
    if (Keys.Num() == 0) return FText::GetEmpty();
    // arrows as arrows, everything else by its short name ("Left Shift" -> "Shift", "Space Bar" -> "Space")
    auto Label = [](const FKey& Key) -> FString
    {
        if (Key == EKeys::Left) return TEXT("\u2190");
        if (Key == EKeys::Up) return TEXT("\u2191");
        if (Key == EKeys::Right) return TEXT("\u2192");
        if (Key == EKeys::Down) return TEXT("\u2193");
        if (Key == EKeys::SpaceBar) return TEXT("Space");
        // either Shift (or Ctrl, Alt) works, so the side isn't named
        FString Name = Key.GetDisplayName(false).ToString();
        if (Key.IsModifierKey()) Name.RemoveFromStart(TEXT("Left ")) || Name.RemoveFromStart(TEXT("Right "));
        return Name;
    };
    // several letter keys (movement) read as one group, in keyboard order: W, A, S, D -> "WASD"
    static const FString Layout = TEXT("QWERTYUIOPASDFGHJKLZXCVBNM");
    TArray<FString> Letters;
    for (const FKey& Key : Keys)
    {
        const FString Name = Label(Key);
        if (Name.Len() == 1 && Layout.Contains(Name)) Letters.Add(Name);
    }
    if (Letters.Num() > 1)
    {
        Letters.Sort([](const FString& A, const FString& B) { return Layout.Find(A) < Layout.Find(B); });
        return FText::FromString(FString::Join(Letters, TEXT("")));
    }
    return FText::FromString(Label(Keys[0]));
}
