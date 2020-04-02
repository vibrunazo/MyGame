// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "Level/LevelBuilder.h"



ALevelBuilder* UMyGameInstance::GetLevelBuilder()
{
    return LevelBuilderRef;
}
	
    
void UMyGameInstance::SetLevelBuilderRef(class ALevelBuilder* NewLevelBuilder)
{
    LevelBuilderRef = NewLevelBuilder;
}