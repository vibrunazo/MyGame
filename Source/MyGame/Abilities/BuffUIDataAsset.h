// How a buff or debuff shows up in the UI (the duration bars under health bars). One asset per buff is the
// single source of this; gameplay effects point at it through their UMyGameplayEffectUIData component.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Styling/SlateBrush.h"
#include "BuffUIDataAsset.generated.h"

UCLASS(BlueprintType)
class MYGAME_API UBuffUIDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff UI")
	FText DisplayName;

	// For a future mouse-over tooltip
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff UI", meta = (MultiLine = true))
	FText Description;

	// Tints the icon and the duration bar
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff UI")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff UI")
	FSlateBrush Icon;
};
