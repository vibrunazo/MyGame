// Dev command `MyGame.MenuToRunTest [quit]`: from the main menu into a run with a real key, as a player does, and the
// run takes the keyboard. It runs in any build but Shipping, the cooked game included: the cooked game's dead keyboard
// (a level left in the menu's UI Only input) happened where the editor's tests never go.
//
// Steps, on the core ticker (it keeps running across map loads):
//   menu   back to the main menu if needed; it shows, in UI Only input
//   start  Enter, through Slate as a real key, presses the menu's focused button: the run's level loads
//   run    the floor is ready; the input mode is Game Only
//   move   W held 0.5 s, through Slate, moves the player
// Each problem is a `MenuToRun: ...` Error line, then the verdict `MenuToRun: PASS (N checks)` or
// `MenuToRun: FAIL (N problems, N checks)`; with `quit` the game then exits, 0 on a pass, 1 on a fail.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/OutputDeviceRedirector.h"
#include "UObject/UObjectIterator.h"
#include "../Level/FloorBuilder.h"
#include "../Level/RunSubsystem.h"
#include "../MyGameInstance.h"
#include "../UI/MainMenu.h"

namespace MenuToRun
{
	const TCHAR* MenuMap = TEXT("MainMenu");
	const TCHAR* RunMap = TEXT("LevelPersistent");
	constexpr double StepLimit = 60.0;   // real seconds any one step may take
	constexpr double HoldSeconds = 0.5;
	constexpr double MinMoved = 50.0;

	enum class EStep { ToMenu, Menu, Press, Started, Run, Holding, Done };

	struct FTest
	{
		EStep Step = EStep::ToMenu;
		double InStep = 0.0;
		bool bQuit = false;
		bool bAskedForMenu = false;
		int32 Checks = 0;
		TArray<FString> Problems;
		FVector Before = FVector::ZeroVector;
		FTSTicker::FDelegateHandle Handle;
	};
	TUniquePtr<FTest> Test;

	UWorld* World()
	{
		return GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
	}

	FString MapName(UWorld* W)
	{
		return W ? UGameplayStatics::GetCurrentLevelName(W, true) : FString();
	}

	FString InputMode(UWorld* W)
	{
#if UE_ENABLE_DEBUG_DRAWING
		if (APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr) return PC->GetCurrentInputModeDebugString();
#endif
		return FString();
	}

	void Key(const FKey& K, bool bDown)
	{
		FSlateApplication& App = FSlateApplication::Get();
		const FKeyEvent Event(K, FModifierKeysState(), App.GetUserIndexForKeyboard(), false, 0, 0);
		if (bDown) App.ProcessKeyDownEvent(Event);
		else App.ProcessKeyUpEvent(Event);
	}

	void Check(bool bOk, const FString& Problem)
	{
		++Test->Checks;
		if (!bOk) Test->Problems.Add(Problem);
	}

	void Next(EStep Step)
	{
		Test->Step = Step;
		Test->InStep = 0.0;
	}

	void Finish()
	{
		for (const FString& P : Test->Problems) UE_LOG(LogTemp, Error, TEXT("MenuToRun: %s"), *P);
		const bool bOk = Test->Problems.IsEmpty();
		if (bOk)
		{
			UE_LOG(LogTemp, Display, TEXT("MenuToRun: PASS (%d checks)"), Test->Checks);
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("MenuToRun: FAIL (%d problems, %d checks)"), Test->Problems.Num(), Test->Checks);
		}
		GLog->Flush();
		Next(EStep::Done);
		// forced: on Windows a graceful exit request never reaches the process's exit code (GuardedMain returns its
		// init result, so the game exits 0 whatever the verdict). This flushes the log and ends the process with the code;
		// a test run has its own saved folder, so the skipped shutdown loses nothing
		if (Test->bQuit) FPlatformMisc::RequestExitWithStatus(true, bOk ? 0 : 1);
	}

	bool Tick(float DeltaSeconds)
	{
		if (!Test || Test->Step == EStep::Done) return false;
		Test->InStep += DeltaSeconds;
		UWorld* W = World();
		auto TimedOut = [](const TCHAR* What)
		{
			if (Test->InStep < StepLimit) return false;
			Test->Problems.Add(FString::Printf(TEXT("timed out after %.0f s waiting for %s"), StepLimit, What));
			Finish();
			return true;
		};
		switch (Test->Step)
		{
		case EStep::ToMenu:
			if (MapName(W) == MenuMap) { Next(EStep::Menu); break; }
			if (!Test->bAskedForMenu)
			{
				if (UMyGameInstance* GI = W ? Cast<UMyGameInstance>(W->GetGameInstance()) : nullptr) GI->QuitToMainMenu();
				Test->bAskedForMenu = true;
			}
			TimedOut(TEXT("the main menu map"));
			break;
		case EStep::Menu:
		{
			bool bShown = false;
			for (TObjectIterator<UMainMenu> It; It; ++It) bShown |= It->GetWorld() == W && It->IsInViewport();
			// the menu focuses its first button a frame after it shows: give it a moment
			if (bShown && Test->InStep > 0.5)
			{
				const FString Mode = InputMode(W);
				Check(Mode.IsEmpty() || Mode.StartsWith(TEXT("UI Only")), FString::Printf(TEXT("the menu's input mode is '%s', not UI Only"), *Mode));
				Key(EKeys::Enter, true);
				Next(EStep::Press);
			}
			else TimedOut(TEXT("the main menu to show"));
			break;
		}
		case EStep::Press:
			Key(EKeys::Enter, false);
			Next(EStep::Started);
			break;
		case EStep::Started:
			if (MapName(W) == RunMap) { ++Test->Checks; Next(EStep::Run); }
			else if (Test->InStep >= 15.0)
			{
				Test->Problems.Add(FString::Printf(TEXT("Enter on the main menu started no run in 15 s (still on '%s'): no focused button?"), *MapName(W)));
				Finish();
			}
			break;
		case EStep::Run:
		{
			AFloorBuilder* Floor = W ? URunSubsystem::GetFloorBuilder(W) : nullptr;
			APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
			APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			if (Floor && Floor->AreTilesReady() && Pawn && Test->InStep > 1.0)
			{
				const FString Mode = InputMode(W);
				Check(Mode.IsEmpty() || Mode.StartsWith(TEXT("Game Only")), FString::Printf(TEXT("the run's input mode is '%s', not Game Only: keys and pads won't reach the game"), *Mode));
				Test->Before = Pawn->GetActorLocation();
				Key(EKeys::W, true);
				Next(EStep::Holding);
			}
			else TimedOut(TEXT("the floor to be ready"));
			break;
		}
		case EStep::Holding:
			if (Test->InStep >= HoldSeconds)
			{
				Key(EKeys::W, false);
				APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
				const double Moved = PC && PC->GetPawn() ? FVector::Dist2D(PC->GetPawn()->GetActorLocation(), Test->Before) : 0.0;
				Check(Moved >= MinMoved, FString::Printf(TEXT("W held %.1f s moved the player %.0f, not %.0f or more: the keyboard doesn't reach the game"), HoldSeconds, Moved, MinMoved));
				Finish();
			}
			break;
		default:
			break;
		}
		return Test->Step != EStep::Done;
	}

	void Start(const TArray<FString>& Args)
	{
		if (Test && Test->Handle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(Test->Handle);
		Test = MakeUnique<FTest>();
		Test->bQuit = Args.Contains(TEXT("quit"));
		UE_LOG(LogTemp, Display, TEXT("MenuToRun: started"));
		Test->Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick));
	}

	FAutoConsoleCommand Command(
		TEXT("MyGame.MenuToRunTest"),
		TEXT("From the main menu into a run with a real Enter, then checks the run takes the keyboard (Game Only, W moves). "
		     "Ends with a 'MenuToRun: PASS|FAIL' line; 'quit' exits afterwards with 0 or 1."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Start));
}

#endif
