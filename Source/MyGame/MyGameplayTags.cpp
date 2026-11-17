#include "MyGameplayTags.h"

namespace MyGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Attacking, "state.attacking");
	UE_DEFINE_GAMEPLAY_TAG(Combo_CanCancel, "combo.cancancel");

	UE_DEFINE_GAMEPLAY_TAG(Status_HitStun, "Ass.Status.HitStun");
	UE_DEFINE_GAMEPLAY_TAG(Status_StunImmune, "Ass.Status.StunImmune");
	UE_DEFINE_GAMEPLAY_TAG(Status_NoControl, "Ass.Status.NoControl");
	UE_DEFINE_GAMEPLAY_TAG(Status_NoMove, "Ass.Status.NoMove");
	UE_DEFINE_GAMEPLAY_TAG(Status_NoPawnBlock, "Ass.Status.NoPawnBlock");
	UE_DEFINE_GAMEPLAY_TAG(Status_ImmuneProjectile, "Ass.Status.ImmuneProjectile");
	UE_DEFINE_GAMEPLAY_TAG(Status_Airborne, "Ass.Status.Airborne");
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
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hit_Impact, "GameplayCue.hit.impact");

	UE_DEFINE_GAMEPLAY_TAG(Input_Release_0, "input.release.0");
	UE_DEFINE_GAMEPLAY_TAG(Input_Release_1, "input.release.1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Release_2, "input.release.2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Release_3, "input.release.3");

	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "data.damage");
	UE_DEFINE_GAMEPLAY_TAG(Data_HitStun, "Ass.Data.HitStun");
	UE_DEFINE_GAMEPLAY_TAG(Data_Knockback, "Ass.Data.Knockback");
	UE_DEFINE_GAMEPLAY_TAG(Data_CamShake, "Ass.Data.CamShake");
	UE_DEFINE_GAMEPLAY_TAG(Data_Launch, "Ass.Data.Launch");
	UE_DEFINE_GAMEPLAY_TAG(Data_Launch_X, "Ass.Data.Launch.X");
	UE_DEFINE_GAMEPLAY_TAG(Data_Launch_Y, "Ass.Data.Launch.Y");
	UE_DEFINE_GAMEPLAY_TAG(Data_Launch_Z, "Ass.Data.Launch.Z");
	UE_DEFINE_GAMEPLAY_TAG(Data_StunImmune, "Ass.Data.StunImmune");
	UE_DEFINE_GAMEPLAY_TAG(Data_NoControl, "Ass.Data.NoControl");
}
