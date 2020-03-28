// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "MyCharacter.h"
#include "MyDefaultPawn.h"

void AMyPlayerController::OnCharDies(AMyCharacter* CharRef)
{
    FVector Loc = CharRef->GetActorLocation();
    FActorSpawnParameters params;
	AMyDefaultPawn* MyPawn = GetWorld()->SpawnActor<AMyDefaultPawn>(AMyDefaultPawn::StaticClass(), Loc, FRotator::ZeroRotator, params);
    DefaultPawnRef = MyPawn;
    SetViewTargetWithBlend(DefaultPawnRef, 3.0f);
}

void AMyPlayerController::OnDelayedCharDies(AMyCharacter* CharRef)
{
    Possess(DefaultPawnRef);

}