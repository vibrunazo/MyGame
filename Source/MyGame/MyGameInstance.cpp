// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "Level/LevelBuilder.h"
#include "Engine/World.h"
#include "Player/MyCharacter.h"


UMyGameInstance::UMyGameInstance()
{
}

void UMyGameInstance::Init()
{
    Super::Init();
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

    // TODO this might be called after the game is over, before the new game actually starts, which might cause random stream to desync?
    RandomStream = FRandomStream(RandomSeed);
	if (!RandomSeed) RandomStream.GenerateNewSeed();
    UE_LOG(LogTemp, Warning, TEXT("GameInstance Random Seed: %d"), RandomStream.GetCurrentSeed());
	UE_LOG(LogTemp, Warning, TEXT("Random numbers: %d, %d, %d"), RandomStream.RandRange(0, 10), RandomStream.RandRange(0, 10), RandomStream.RandRange(0, 10));
}

void UMyGameInstance::LevelClear(FString NextMapUrl)
{
    LevelDifficulty++;
    UWorld* LeMundi = GetWorld();
	if (!LeMundi) return;
	LeMundi->ServerTravel(NextMapUrl);
}

void UMyGameInstance::StoreCharStats()
{
    if (PlayerCharRef) Health = PlayerCharRef->GetAttributes()->GetHealth();
}

void UMyGameInstance::SetLevelBuilderRef(class ALevelBuilder* NewLevelBuilder)
{
    LevelBuilderRef = NewLevelBuilder;
}