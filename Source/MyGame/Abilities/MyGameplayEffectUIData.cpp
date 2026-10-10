// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameplayEffectUIData.h"

#include "BuffUIDataAsset.h"

FBuffUI UMyGameplayEffectUIData::GetBuffUI() const
{
	FBuffUI UI;
	if (!BuffUIAsset) return UI;
	UI.Name = FName(*BuffUIAsset->DisplayName.ToString());
	UI.Description = FName(*BuffUIAsset->Description.ToString());
	UI.Color = BuffUIAsset->Color;
	UI.Icon = BuffUIAsset->Icon;
	return UI;
}
