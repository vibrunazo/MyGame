// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "Level/LevelBuilder.h"
#include "Player/MyCharacter.h"
#include "Props/ItemDataAsset.h"
#include "Level/RoomCameraPawn.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"


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

TArray<FString> UMyGameInstance::GetItemsICannotGetMoreOf()
{
    TArray<FString> result = TArray<FString>();
    TMap<FString, int32> ItemCount = TMap<FString, int32>();
    for (auto&& Item : Inventory)
    {
        UE_LOG(LogTemp, Warning, TEXT("inventory item: %s"), *Item->ItemName.ToString());
        int32 ThisCount = 0;
        if (ItemCount.Contains(Item->ItemName.ToString()))
        {
            ThisCount = *ItemCount.Find(Item->ItemName.ToString());
            UE_LOG(LogTemp, Warning, TEXT("already exists, have: %d of them"), ThisCount);
            ItemCount.Emplace(Item->ItemName.ToString(), ++ThisCount);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("New Item, adding to count"));
            ThisCount = 1;
            ItemCount.Add(Item->ItemName.ToString(), 1);
        }
        if (ThisCount >= Item->MaxStacks && Item->MaxStacks > 0)
        {
            result.Add(Item->ItemName.ToString());
        }
    }

    return result;
}

void UMyGameInstance::TeleportPlayer(FVector NewLocation)
{
    if (!PlayerCharRef) return;
    PlayerCharRef->SetActorLocation(NewLocation);
}

void UMyGameInstance::DoCamShake(float Intensity)
{
    if (!CamShakeClass || !PlayerCharRef || !PlayerCharRef->CamPawnRef) return;
    FVector Loc = PlayerCharRef->CamPawnRef->GetActorLocation();
    Loc.X += (100 - Intensity);
    UGameplayStatics::PlayWorldCameraShake(GetWorld(), CamShakeClass, Loc, 0.0f, 100.f);
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