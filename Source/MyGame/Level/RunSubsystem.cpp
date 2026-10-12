#include "RunSubsystem.h"
#include "FloorBuilder.h"
#include "Engine/World.h"

AFloorBuilder* URunSubsystem::GetFloorBuilder(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const URunSubsystem* Run = World ? World->GetSubsystem<URunSubsystem>() : nullptr;
	return Run ? Run->FloorBuilder.Get() : nullptr;
}
