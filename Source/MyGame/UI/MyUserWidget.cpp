// Fill out your copyright notice in the Description page of Project Settings.


#include "MyUserWidget.h"
#include "../MyGameInstance.h"
#include "../Player/MyCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Containers/Ticker.h"

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
float UMyUserWidget::GetAttackSpeedAttr()
{
    UGameInstance* GI = GetGameInstance();
    UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI);
    if (MyGI && MyGI->PlayerCharRef && MyGI->PlayerCharRef->GetAttributes())
    {
        return MyGI->PlayerCharRef->GetAttributes()->GetAttackSpeed();
    }
    return 0.0f;
}

void UMyUserWidget::FocusFirstButton()
{
    // next frame: a widget only takes focus once it's in the viewport's hierarchy
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
    {
        UButton* First = nullptr;
        if (WidgetTree)
        {
            WidgetTree->ForEachWidget([&First](UWidget* Widget)
            {
                UButton* Button = Cast<UButton>(Widget);
                if (!First && Button && Button->GetIsEnabled() && Button->IsVisible() && Button->GetIsFocusable()) First = Button;
            });
        }
        if (First)
        {
            if (APlayerController* PC = GetOwningPlayer()) First->SetUserFocus(PC);
            else First->SetKeyboardFocus();
        }
        return false;  // once
    }));
}
