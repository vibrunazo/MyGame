// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../BaseController.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API AMyPlayerController : public ABaseController
{
	GENERATED_BODY()

public:
	void OnCharDies(class AMyCharacter* CharRef);
	void OnDelayedCharDies(class AMyCharacter* CharRef);

	class AMyDefaultPawn* DefaultPawnRef;
};
