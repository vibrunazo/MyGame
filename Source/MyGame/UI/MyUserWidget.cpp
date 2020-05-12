// Fill out your copyright notice in the Description page of Project Settings.


#include "MyUserWidget.h"
#include "../MyGameInstance.h"
#include "../Player/MyCharacter.h"

UMyGameInstance* UMyUserWidget::GetMyGameInstance()
{
    UGameInstance* GI = GetGameInstance();
    UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
    return MyGI;
}

TArray<UItemDataAsset*> UMyUserWidget::GetInventory()
{
    UGameInstance* GI = GetGameInstance();
    UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
    if (MyGI)
    {
        return MyGI->Inventory;
    }
    return {};
}

float UMyUserWidget::GetMaxHealthAttr()
{
    UGameInstance* GI = GetGameInstance();
    UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
    if (MyGI && MyGI->PlayerCharRef && MyGI->PlayerCharRef->GetAttributes())
    {
         return MyGI->PlayerCharRef->GetAttributes()->GetMaxHealth();
    }
    return 0.0f;
}
float UMyUserWidget::GetAttackAttr()
{
    UGameInstance* GI = GetGameInstance();
    UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
    if (MyGI && MyGI->PlayerCharRef && MyGI->PlayerCharRef->GetAttributes())
    {
         return MyGI->PlayerCharRef->GetAttributes()->GetAttack();
    }
    return 0.0f;
}
float UMyUserWidget::GetDefenseAttr()
{
    UGameInstance* GI = GetGameInstance();
    UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
    if (MyGI && MyGI->PlayerCharRef && MyGI->PlayerCharRef->GetAttributes())
    {
         return MyGI->PlayerCharRef->GetAttributes()->GetDefense();
    }
    return 0.0f;
}
float UMyUserWidget::GetSpeedAttr()
{
    UGameInstance* GI = GetGameInstance();
    UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
    if (MyGI && MyGI->PlayerCharRef && MyGI->PlayerCharRef->GetAttributes())
    {
         return MyGI->PlayerCharRef->GetAttributes()->GetSpeed();
    }
    return 0.0f;
}