// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "Level/LevelBuilder.h"
#include "Engine/World.h"
#include "Player/MyCharacter.h"


ALevelBuilder* UMyGameInstance::GetLevelBuilder()
{
    return LevelBuilderRef;
}
	
    
void UMyGameInstance::SetCharRef(AMyCharacter* NewRef)
{
    PlayerCharRef = NewRef;
}

void UMyGameInstance::OnGameOver()
{
    LevelDifficulty = 1;
    Health = 100.f;
}

void UMyGameInstance::LevelClear(FString NextMapUrl)
{
    LevelDifficulty++;
    if (PlayerCharRef) Health = PlayerCharRef->GetAttributes()->GetHealth();
    UWorld* LeMundi = GetWorld();
	if (!LeMundi) return;
	LeMundi->ServerTravel(NextMapUrl);
}

void UMyGameInstance::SetLevelBuilderRef(class ALevelBuilder* NewLevelBuilder)
{
    LevelBuilderRef = NewLevelBuilder;
}