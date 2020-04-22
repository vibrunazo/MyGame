// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
// #include "UI/MyUserWidget.h"
#include "BaseController.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API ABaseController : public APlayerController
{
	GENERATED_BODY()

public:
	void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseController)
	TSubclassOf<class UMyUserWidget> InitialWidget;
	class UMyUserWidget* InitialWidgetRef;

	
};
