// This floor's run state. It lives and dies with the world, so nothing points into a world that travel tore down.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RunSubsystem.generated.h"

class ALevelBuilder;

UCLASS()
class MYGAME_API URunSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// The level builder of this world; null outside a generated level
	UFUNCTION(BlueprintPure, Category = "Run", meta = (WorldContext = "WorldContextObject"))
	static ALevelBuilder* GetLevelBuilder(const UObject* WorldContextObject);

	void SetLevelBuilder(ALevelBuilder* Builder) { LevelBuilder = Builder; }

private:
	TWeakObjectPtr<ALevelBuilder> LevelBuilder;
};
