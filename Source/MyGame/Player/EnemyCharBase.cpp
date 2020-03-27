// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharBase.h"

void AEnemyCharBase::SetDefaultProperties()
{
    Super::SetDefaultProperties();
    bUseControllerRotationYaw = true;
    Team = 1;
}