#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "../Abilities/MyGameplayEffectUIData.h"
#include "AbilityUIDataAsset.generated.h"

class UGameplayAbility;

/**
 * Each ability's UI (name, description, color, icon), for the HUD's buttons and the ability books. MyGame's, not the
 * ability class's (Ass Plan.md Step 1, item 6). Read only through UMyBlueprintFunctionLibrary::GetAbilityUI: in
 * Step 4 the entries move into the ability templates (4.2's presentation row) and the function reads those instead.
 */
UCLASS(BlueprintType)
class MYGAME_API UAbilityUIDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Ability UI")
	TMap<TSubclassOf<UGameplayAbility>, FBuffUI> Abilities;
};
