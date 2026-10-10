#include "MyGameplayTags.h"

namespace MyGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Attacking, "state.attacking");
	UE_DEFINE_GAMEPLAY_TAG(Combo_CanCancel, "combo.cancancel");

	UE_DEFINE_GAMEPLAY_TAG(Status_HitStun, "status.hitstun");
	UE_DEFINE_GAMEPLAY_TAG(Status_StunImmune, "status.stunimmune");
	UE_DEFINE_GAMEPLAY_TAG(Status_NoControl, "status.nocontrol");
	UE_DEFINE_GAMEPLAY_TAG(Status_NoMove, "status.nomove");
	UE_DEFINE_GAMEPLAY_TAG(Status_NoPawnBlock, "status.nopawnblock");
	UE_DEFINE_GAMEPLAY_TAG(Status_ImmuneProjectile, "status.immune.proj");
	UE_DEFINE_GAMEPLAY_TAG(Status_Health_75, "status.health.75");
	UE_DEFINE_GAMEPLAY_TAG(Status_Health_70, "status.health.70");
	UE_DEFINE_GAMEPLAY_TAG(Status_Health_50, "status.health.50");
	UE_DEFINE_GAMEPLAY_TAG(Status_Health_30, "status.health.30");
	UE_DEFINE_GAMEPLAY_TAG(Status_Health_25, "status.health.25");

	UE_DEFINE_GAMEPLAY_TAG(Activate_GroundAttack, "activate.groundattack");
	UE_DEFINE_GAMEPLAY_TAG(Activate_FlyingAttack, "activate.flyingattack");

	UE_DEFINE_GAMEPLAY_TAG(Notify_Hit_Start, "notify.hit.start");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Hit_End, "notify.hit.end");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Hit_Connect, "notify.hit.connect");
	UE_DEFINE_GAMEPLAY_TAG(Notify_Projectile_Cast, "notify.projectile.cast");

	UE_DEFINE_GAMEPLAY_TAG(Event_Dash, "event.dash");

	UE_DEFINE_GAMEPLAY_TAG(Input_Release_0, "input.release.0");
	UE_DEFINE_GAMEPLAY_TAG(Input_Release_1, "input.release.1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Release_2, "input.release.2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Release_3, "input.release.3");

	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "data.damage");
	UE_DEFINE_GAMEPLAY_TAG(Data_HitStun, "data.hitstun");
	UE_DEFINE_GAMEPLAY_TAG(Data_Knockback, "data.knockback");
	UE_DEFINE_GAMEPLAY_TAG(Data_CamShake, "data.camshake");
	UE_DEFINE_GAMEPLAY_TAG(Data_Launch, "data.launch");
	UE_DEFINE_GAMEPLAY_TAG(Data_Launch_X, "data.launch.x");
	UE_DEFINE_GAMEPLAY_TAG(Data_Launch_Y, "data.launch.y");
	UE_DEFINE_GAMEPLAY_TAG(Data_Launch_Z, "data.launch.z");
	UE_DEFINE_GAMEPLAY_TAG(Data_NoApply, "data.noapply");
	UE_DEFINE_GAMEPLAY_TAG(Data_StunImmune, "data.stunimmune");
	UE_DEFINE_GAMEPLAY_TAG(Data_NoControl, "data.nocontrol");
}
