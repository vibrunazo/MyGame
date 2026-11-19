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

	/**
	 * For the AI suite's decision matrix (Ass Step 7, Q6): the tree task each command stands for, by its index in
	 * Definition's commands, as the strict recorder names a tree's task ("Move To[34]"). "A>B": A for the command's first
	 * frame, then B (the spot move: the tree's query runs a frame before its move). Commands not listed show no task.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pattern")
	TMap<int32, FString> TreeTaskNames;
};
