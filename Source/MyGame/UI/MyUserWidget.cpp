// Fill out your copyright notice in the Description page of Project Settings.


#include "MyUserWidget.h"
#include "../MyGameInstance.h"

UMyGameInstance* UMyUserWidget::GetMyGameInstance()
{
    UGameInstance* GI = GetGameInstance();
    UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
    return MyGI;
}