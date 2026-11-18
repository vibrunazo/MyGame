// MyGame's native gameplay tags. The plugin's (Ass.*, GameplayCue.Ass.*) are AssCore's AssTags; data-only tags (buffs,
// cues, item tags) stay in DefaultGameplayTags.ini.

#pragma once

#include "NativeGameplayTags.h"

namespace MyGameplayTags
{
	// The plugin's combat reads these through its settings (DefaultGame.ini, UAssCombatSettings): the ground-only and
	// air-only abilities
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activate_GroundAttack);
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activate_FlyingAttack);

	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Health_75);
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Health_70);
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Health_50);
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Health_30);
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Health_25);

	// Sent to a character to make it dash; GA_Dash triggers on it
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Dash);

	// Released ability buttons, sent as gameplay events (index = EInput: Punch, Kick, Cast, Jump)
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Release_0);
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Release_1);
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Release_2);
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Release_3);

	// The damage's set-by-caller magnitude
	MYGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
}
