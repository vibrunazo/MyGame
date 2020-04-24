// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "MyCharacter.h"
#include "Blueprint/UserWidget.h"
#include "MyDefaultPawn.h"
#include "../UI/MyUserWidget.h"
#include "../MyGameInstance.h"
#include "Kismet/GameplayStatics.h"

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    check(InputComponent);
	InputComponent->BindAction("PauseMenu", IE_Pressed, this, &AMyPlayerController::OnPausePressed).bExecuteWhenPaused = true;
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
        if (PauseWidgetRef) PauseWidgetRef->RemoveFromParent();
        UGameplayStatics::SetGamePaused(GetWorld(), false);
        bIsPaused = false;

    }
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