// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomDataAsset.h"


FName URoomDataAsset::GetAutoLevelAddress()
{
    FString NameString = GetName();
    NameString = NameString.RightChop(3);
    NameString = "Game/Maps/Rooms/" + NameString;
    FName Result = FName(*NameString);
    return Result;
}