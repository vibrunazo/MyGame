// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "Player/MyCharacter.h"
#include "Props/ItemDataAsset.h"
#include "Level/RoomCamera.h"
#include "Level/FloorBuilder.h"
#include "Level/RunSubsystem.h"

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

TArray<struct FAbilityStruct> UMyGameInstance::GetPlayerAbiliies()
{
    TArray<struct FAbilityStruct> result = TArray<struct FAbilityStruct>();
    if (!PlayerCharRef) return result;
    return PlayerCharRef->Abilities;
}

void UMyGameInstance::TeleportPlayer(FVector NewLocation)
{
    if (!PlayerCharRef) return;
    PlayerCharRef->SetActorLocation(NewLocation);
}

void UMyGameInstance::DoCamShake(float Intensity)
{
    if (!CamShakeClass || !PlayerCharRef || !PlayerCharRef->CamPawnRef) return;
    FVector Loc = PlayerCharRef->CamPawnRef->GetCameraLoc();
    Loc.X += (100 - Intensity);
    UGameplayStatics::PlayWorldCameraShake(GetWorld(), CamShakeClass, Loc, 0.0f, 100.f);
}

void UMyGameInstance::StartNewRun()
{
    NewGame();
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, RunMap);
}

void UMyGameInstance::NextFloor()
{
    FloorNumber++;
    SeedFloor();
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, RunMap);
}

void UMyGameInstance::QuitToMainMenu()
{
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, MainMenuMap);
}

void UMyGameInstance::NewGame()
{
    FloorNumber = 1;
    Health = 100.f;
    Mana = 40.f;
    Inventory.Empty();
    CarriedAbilities.Empty();

    // RandomSeed when set (tests set it before StartNewRun), else -Seed=N on the command line (replaying a run for a bug
    // report), else a new one. -Floor=N starts on floor N (dev: replaying a later floor)
    int32 Seed = RandomSeed;
    if (!Seed) FParse::Value(FCommandLine::Get(), TEXT("Seed="), Seed);
    if (!Seed) Seed = FMath::Max(1, FMath::Rand());
    RunSeed = Seed;
    int32 StartFloor = 1;
    if (FParse::Value(FCommandLine::Get(), TEXT("Floor="), StartFloor)) FloorNumber = (uint8)FMath::Clamp(StartFloor, 1, 255);
    SeedFloor();
}

void UMyGameInstance::SeedFloor()
{
    RandomStream = MakeFloorStream();
    UE_LOG(LogTemp, Warning, TEXT("Run seed %d, floor %d: stream seed %d"), RunSeed, FloorNumber, RandomStream.GetInitialSeed());
}

FRandomStream UMyGameInstance::MakeFloorStream() const
{
    // floor 1 keeps the plain seed, so the seeded baselines stay valid
    FRandomStream Stream(FloorNumber <= 1 ? RunSeed : (int32)HashCombine(GetTypeHash(RunSeed), GetTypeHash((int32)FloorNumber)));
    // three draws the floor layouts were made after (the baselines depend on them)
    for (int32 Draw = 0; Draw < 3; ++Draw) Stream.RandRange(0, 10);
    return Stream;
}

int32 UMyGameInstance::TakeStartRoom()
{
    int32 Room = -1;
    if (bStartRoomTaken || !FParse::Value(FCommandLine::Get(), TEXT("Room="), Room)) return -1;
    bStartRoomTaken = true;
    return Room;
}

void UMyGameInstance::GoToRoom(int32 Room)
{
    AFloorBuilder* Builder = URunSubsystem::GetFloorBuilder(this);
    if (!Builder || !Builder->MoveToRoom(Room)) UE_LOG(LogTemp, Warning, TEXT("GoToRoom %d: no such room, or no free spot in it"), Room);
}

void UMyGameInstance::StoreCharStats()
{
    if (PlayerCharRef)
    {
        Health = PlayerCharRef->GetAttributes()->GetHealth();
        Mana = PlayerCharRef->GetAttributes()->GetMana();
        CarriedAbilities = PlayerCharRef->Abilities;
    }
}
