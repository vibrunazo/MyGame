// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "Level/LevelBuilder.h"
#include "Engine/World.h"
#include "Player/MyCharacter.h"


UMyGameInstance::UMyGameInstance()
{
    NewGame();
}

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
    NewGame();
}

void UMyGameInstance::NewGame()
{
    LevelDifficulty = 1;
    Health = 100.f;
    Inventory.Empty();
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