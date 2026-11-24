#include "MyGameplayTags.h"

namespace MyGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Activate_GroundAttack, "activate.groundattack");
	UE_DEFINE_GAMEPLAY_TAG(Activate_FlyingAttack, "activate.flyingattack");

	UE_DEFINE_GAMEPLAY_TAG(Status_Health_75, "status.health.75");
	UE_DEFINE_GAMEPLAY_TAG(Status_Health_70, "status.health.70");
	UE_DEFINE_GAMEPLAY_TAG(Status_Health_50, "status.health.50");
	UE_DEFINE_GAMEPLAY_TAG(Status_Health_30, "status.health.30");
	UE_DEFINE_GAMEPLAY_TAG(Status_Health_25, "status.health.25");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_BuffedRange, "status.buffedrange", "While on, the AI attacks from its buffed range");

	UE_DEFINE_GAMEPLAY_TAG(Event_Dash, "event.dash");

	UE_DEFINE_GAMEPLAY_TAG(Input_Release_0, "input.release.0");
	UE_DEFINE_GAMEPLAY_TAG(Input_Release_1, "input.release.1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Release_2, "input.release.2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Release_3, "input.release.3");

	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "data.damage");
	UE_DEFINE_GAMEPLAY_TAG(Data_Speed, "data.speed");
}
