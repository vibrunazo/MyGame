// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharBase.h"
#include "Perception/PawnSensingComponent.h"
#include "Components/WidgetComponent.h"




AEnemyCharBase::AEnemyCharBase()
{
    SetDefaultProperties();
}


void AEnemyCharBase::BeginPlay()
{
    Super::BeginPlay();
}

void AEnemyCharBase::SetDefaultProperties()
{
    // UE_LOG(LogTemp, Warning, TEXT("Setting property on enemy"));
    Super::SetDefaultProperties();
    bUseControllerRotationYaw = true;
    Team = 1;
    PawnSenseComp->SightRadius = 1000.0f;
    PawnSenseComp->HearingThreshold = 500.0f;
    PawnSenseComp->LOSHearingThreshold = 500.0f;
    HealthBarComp->SetVisibility(false);
    // HealthBarComp->SetHiddenInGame(true);
}   

void AEnemyCharBase::OnDamaged(AActor* SourceActor)
{
    Super::OnDamaged(SourceActor);
    HealthBarComp->SetVisibility(true);
    // UE_LOG(LogTemp, Warning, TEXT("Enemy damaged"));
}