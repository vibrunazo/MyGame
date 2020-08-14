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

void AEnemyCharBase::OnDamaged(AActor* SourceActor, float Damage, FGameplayEffectSpec Effect)
{
    Super::OnDamaged(SourceActor, Damage, Effect);
    HealthBarComp->SetVisibility(true);

    APawn* SeenPawn = Cast<APawn>(SourceActor);
    if (!SeenPawn) return;
    SetAggroTarget(SeenPawn);
    SetOutline();
    // UE_LOG(LogTemp, Warning, TEXT("Enemy damaged"));
}

/**Sets the outline of the enemy, visible through walls, when the enemy is hit
 * 	The Post process material looks for a custom depth of stencil value 2 or more to draw the outline.
 *  2 is the player outline color, greater is the enemy
 */
void AEnemyCharBase::SetOutline()
{
    GetMesh()->SetRenderCustomDepth(true);
    GetMesh()->SetCustomDepthStencilValue(3);
    GetWorldTimerManager().SetTimer(OutlineTimer, this, &AMyCharacter::RemoveOutline, OutlineRemoveDelay, false);

    TArray<USceneComponent*> ChildrenComponents;
    GetMesh()->GetChildrenComponents(false, ChildrenComponents);

    for (USceneComponent* Component : ChildrenComponents)
    {
        if (!Component->GetOwner()) continue;
        if (UStaticMeshComponent* ChildMesh = Cast<UStaticMeshComponent>(Component))
        {
            ChildMesh->SetRenderCustomDepth(true);
            ChildMesh->SetCustomDepthStencilValue(3);
        }
    }
}


