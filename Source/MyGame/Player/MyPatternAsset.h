#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AssAITypes.h"
#include "MyPatternAsset.generated.h"

/**
 * An enemy's pattern (Ass Plan.md Step 7): the plugin's AI definition, which AMyAIController's driver plays in place of
 * the behavior tree when its Use pattern switch is on.
 */
UCLASS(BlueprintType)
class MYGAME_API UMyPatternAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pattern")
	FAssAIDefinition Definition;
};
