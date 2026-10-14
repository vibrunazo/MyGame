#include "AbilityTask_HitboxWindows.h"
#include "ANS_Hitbox.h"
#include "../MyGameplayTags.h"
#include "../Player/HitBox.h"
#include "../Player/MyCharacter.h"

#include "Animation/AnimMontage.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"

UAbilityTask_HitboxWindows* UAbilityTask_HitboxWindows::WatchHitboxWindows(UGameplayAbility* OwningAbility, TSubclassOf<AHitBox> HitBoxClass)
{
	UAbilityTask_HitboxWindows* Task = NewAbilityTask<UAbilityTask_HitboxWindows>(OwningAbility);
	Task->HitBoxClass = HitBoxClass ? HitBoxClass : TSubclassOf<AHitBox>(AHitBox::StaticClass());
	return Task;
}

void UAbilityTask_HitboxWindows::Activate()
{
	if (!AbilitySystemComponent.IsValid()) return;
	StartHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(MyGameplayTags::Notify_Hit_Start).AddUObject(this, &UAbilityTask_HitboxWindows::HandleWindowStart);
	EndHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(MyGameplayTags::Notify_Hit_End).AddUObject(this, &UAbilityTask_HitboxWindows::HandleWindowEnd);
	ConnectHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(MyGameplayTags::Notify_Hit_Connect).AddUObject(this, &UAbilityTask_HitboxWindows::HandleHitConnect);
}

void UAbilityTask_HitboxWindows::HandleWindowStart(const FGameplayEventData* Payload)
{
	AActor* Avatar = GetAvatarActor();
	if (!IsValid(Avatar)) return;
	DestroyHitbox();
	FActorSpawnParameters Params;
	Params.bNoFail = true;
	Params.Instigator = Cast<APawn>(Avatar);
	Params.Owner = Avatar;
	Hitbox = GetWorld()->SpawnActor<AHitBox>(HitBoxClass, Avatar->GetActorLocation(), FRotator::ZeroRotator, Params);
	Hitbox->AttachToActor(Avatar, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	if (ShouldBroadcastAbilityTaskDelegates()) OnWindowStarted.Broadcast(Hitbox);
	// shapes last: they can hit the moment they're added, and the effects must already be set
	const UANS_Hitbox* Notify = Payload ? Cast<UANS_Hitbox>(Payload->OptionalObject) : nullptr;
	if (!Notify) return;
	// a montage's window is tested at fixed animation times, whatever the frame rate; anything else on overlaps
	AMyCharacter* Char = Cast<AMyCharacter>(Avatar);
	const UAnimMontage* Montage = Cast<UAnimMontage>(Payload->OptionalObject2);
	float Start, End;
	if (Char && Montage && Notify->GetWindow(Montage, Start, End))
	{
		Hitbox->BeginWindow(Char, Montage, Start, End, Notify->Hitboxes, Notify->NumHits, Notify->HitCooldown);
	}
	else
	{
		Hitbox->SetupHitboxes(Notify->Hitboxes, Notify->NumHits, Notify->HitCooldown);
	}
}

void UAbilityTask_HitboxWindows::HandleWindowEnd(const FGameplayEventData* Payload)
{
	// only ANS_Hitbox's own end event (with the notify); the old Blueprint notify's duplicate has no payload
	if (!Payload || !Cast<UANS_Hitbox>(Payload->OptionalObject)) return;
	// the rest of the window is tested before the ability hears it closed (a combo needs to know it connected)
	if (IsValid(Hitbox)) Hitbox->FinishWindow();
	if (ShouldBroadcastAbilityTaskDelegates()) OnWindowEnded.Broadcast();
	DestroyHitbox();
}

void UAbilityTask_HitboxWindows::HandleHitConnect(const FGameplayEventData* Payload)
{
	if (ShouldBroadcastAbilityTaskDelegates()) OnHitConnected.Broadcast();
}

void UAbilityTask_HitboxWindows::DestroyHitbox()
{
	if (IsValid(Hitbox)) Hitbox->Destroy();
	Hitbox = nullptr;
}

void UAbilityTask_HitboxWindows::OnDestroy(bool bInOwnerFinished)
{
	if (AbilitySystemComponent.IsValid())
	{
		if (FGameplayEventMulticastDelegate* D = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(MyGameplayTags::Notify_Hit_Start)) D->Remove(StartHandle);
		if (FGameplayEventMulticastDelegate* D = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(MyGameplayTags::Notify_Hit_End)) D->Remove(EndHandle);
		if (FGameplayEventMulticastDelegate* D = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(MyGameplayTags::Notify_Hit_Connect)) D->Remove(ConnectHandle);
	}
	DestroyHitbox();
	Super::OnDestroy(bInOwnerFinished);
}
