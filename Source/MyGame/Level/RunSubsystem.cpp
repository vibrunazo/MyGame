#include "RunSubsystem.h"
#include "LevelBuilder.h"
#include "Engine/World.h"

ALevelBuilder* URunSubsystem::GetLevelBuilder(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const URunSubsystem* Run = World ? World->GetSubsystem<URunSubsystem>() : nullptr;
	return Run ? Run->LevelBuilder.Get() : nullptr;
}
