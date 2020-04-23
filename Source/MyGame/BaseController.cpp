// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseController.h"
#include "Blueprint/UserWidget.h"
#include "UI/MyUserWidget.h"

void ABaseController::BeginPlay()
{
    Super::BeginPlay();

    if (!InitialWidget) return;
    InitialWidgetRef = CreateWidget<UMyUserWidget>(this, InitialWidget);
    InitialWidgetRef->AddToViewport();
}