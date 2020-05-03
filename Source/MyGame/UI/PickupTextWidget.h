// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyUserWidget.h"
#include "PickupTextWidget.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UPickupTextWidget : public UMyUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Pickup")
	void SetWidgetText(class UItemDataAsset* NewItem);

};
