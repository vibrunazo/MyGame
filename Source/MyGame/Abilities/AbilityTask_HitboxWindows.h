// Owns an attack's hitbox for the whole ability: the montage's ANS_Hitbox notifies open and close hit windows
// (notify.hit.start / notify.hit.end events). Each window spawns a hitbox on the avatar, shaped by that notify,
// and destroys it when the window closes or the ability ends. Connected hits (notify.hit.connect) are reported.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_HitboxWindows.generated.h"

class AHitBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHitboxWindowStarted, AHitBox*, Hitbox);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHitboxWindowEvent);

UCLASS()
class MYGAME_API UAbilityTask_HitboxWindows : public UAbilityTask
{
	GENERATED_BODY()

public:
	// A window opened: the new hitbox has no shapes yet, so the ability can set its effects, sounds and particles first
	UPROPERTY(BlueprintAssignable)
	FHitboxWindowStarted OnWindowStarted;

	// A window closed (its hitbox is destroyed right after)
	UPROPERTY(BlueprintAssignable)
	FHitboxWindowEvent OnWindowEnded;

	// The hitbox hit something
	UPROPERTY(BlueprintAssignable)
	FHitboxWindowEvent OnHitConnected;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_HitboxWindows* WatchHitboxWindows(UGameplayAbility* OwningAbility, TSubclassOf<AHitBox> HitBoxClass);

	virtual void Activate() override;

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void HandleWindowStart(const struct FGameplayEventData* Payload);
	void HandleWindowEnd(const struct FGameplayEventData* Payload);
	void HandleHitConnect(const struct FGameplayEventData* Payload);
	void DestroyHitbox();

	UPROPERTY()
	TSubclassOf<AHitBox> HitBoxClass;

	UPROPERTY()
	TObjectPtr<AHitBox> Hitbox;

	FDelegateHandle StartHandle;
	FDelegateHandle EndHandle;
	FDelegateHandle ConnectHandle;
};
