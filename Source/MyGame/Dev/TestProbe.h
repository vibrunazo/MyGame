#pragma once

#include "CoreMinimal.h"

// Editor-only observation points for the tests' strict recorder (MyGameEditor's UCombatRecorder): what a frame's state
// and the ability system's own events can't show (a hit shape's centre at each sample, a hit, an effect tried on a
// target, a backslide, a camera shake, a sound or particles played from C++). Each is broadcast where the game does it.
// They change nothing the game does, cost one branch when nothing listens, and aren't compiled outside the editor.
#if WITH_EDITOR
struct FTestProbeEvent
{
	const UObject* Who = nullptr;
	FName What;
	const UObject* Other = nullptr;
	FString Detail;
	FVector Value = FVector::ZeroVector;
	double Number = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTestProbe, const FTestProbeEvent&);

struct MYGAME_API FTestProbe
{
	static FOnTestProbe OnEvent;
};

#define MYGAME_TEST_PROBE(...) do { if (FTestProbe::OnEvent.IsBound()) FTestProbe::OnEvent.Broadcast(FTestProbeEvent{__VA_ARGS__}); } while (0)
#else
#define MYGAME_TEST_PROBE(...) do {} while (0)
#endif
