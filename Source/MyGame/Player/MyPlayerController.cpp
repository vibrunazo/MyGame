// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "MyCharacter.h"
#include "MyDefaultPawn.h"
#include "../UI/MyUserWidget.h"
#include "../UI/MyHUDWidget.h"
#include "../MyGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

void AMyPlayerController::BeginPlay()
{
    ShowHUD();
    ShowLevelIntro();
}

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    check(InputComponent);
	InputComponent->BindAction("PauseMenu", IE_Pressed, this, &AMyPlayerController::OnPausePressed).bExecuteWhenPaused = true;

    InputComponent->BindAction("Jump", IE_Pressed, this, &AMyPlayerController::Jump);
    InputComponent->BindAction("Jump", IE_Released, this, &AMyPlayerController::StopJump);
}

void AMyPlayerController::Jump()
{
    SetAbilityKeyDown((EInput)100, true);
    AMyCharacter* MyChar = GetPawn<AMyCharacter>();
    if (MyChar) MyChar->Jump();
}

void AMyPlayerController::StopJump()
{
    SetAbilityKeyDown((EInput)100, false);
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

/// <summary>
/// Tell the HUD to show this ability key as pressed.
/// </summary>
/// <param name="Index">What ability?</param>
/// <param name="IsKeyDown">Is it pressed?</param>
/// <param name="Duration">For how long should it be pressed? If zero, stays pressed until told otherwise. If > 0 then the action button sets a timer to automatically unpress after Duration seconds.</param>
void AMyPlayerController::SetAbilityKeyDown(EInput Index, bool IsKeyDown, float Duration)
{
    if ((uint8)Index < AbilityKeyStates.Num()) AbilityKeyStates[(uint8)Index] = IsKeyDown;
    AMyCharacter* MyChar = GetPawn<AMyCharacter>();
    if (MyChar) MyChar->SetAbilityKeyDown((uint8)Index, IsKeyDown);
    if (HUDWidgetRef) HUDWidgetRef->BPUpdateAbilityKey((uint8)Index, IsKeyDown, Duration);
}

bool AMyPlayerController::IsAbilityKeyDown(uint8 Index)
{
    if (Index < AbilityKeyStates.Num()) return AbilityKeyStates[Index];
    return false;
}

void AMyPlayerController::ShowAbilityCooldown(uint8 Index, float Cooldown)
{
    if (HUDWidgetRef) HUDWidgetRef->BPUpdateCooldown(Index, Cooldown);
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