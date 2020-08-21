// Fill out your copyright notice in the Description page of Project Settings.


#include "MyHealthBar.h"

#include "GameplayEffect.h"

void UMyHealthBar::GetRatioFromHealth(float NewHealth)
{

}

float UMyHealthBar::GetHealth()
{
    return Health;
}

/// <summary>
/// Adds a new Duration Bar to the panel of bars in this Health Bar.
/// </summary>
/// <param name="Duration"></param>
/// <param name="ActiveEffectHandle"></param>
void UMyHealthBar::AddDurationToHealthBar(float Duration, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
    UUserWidget** OldWidget = MapOfBars.Find(ActiveEffectHandle);
    if (OldWidget)
    {
        BP_ResetDurationBar(Duration, *OldWidget);
        return;
    }
    UMyGameplayEffectUIData* MyUI = Cast<UMyGameplayEffectUIData>(EffectSpec.Def->UIData);
    UUserWidget* NewWidget = BP_AddNewDurationBar(Duration, MyUI);
    MapOfBars.Add(ActiveEffectHandle, NewWidget);
}
