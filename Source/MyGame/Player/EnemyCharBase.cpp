// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharBase.h"
#include "Perception/PawnSensingComponent.h"

void AEnemyCharBase::SetDefaultProperties()
{
    Super::SetDefaultProperties();
    bUseControllerRotationYaw = true;
    Team = 1;
    PawnSenseComp->SightRadius = 1000.0f;
    PawnSenseComp->HearingThreshold = 500.0f;
    PawnSenseComp->LOSHearingThreshold = 500.0f;
}