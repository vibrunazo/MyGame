// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ReportDeath.generated.h"


//DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FDieSignature, IReportDeath, OnDieDelegate, class AMyCharacter*, WhoDied);
DECLARE_MULTICAST_DELEGATE_OneParam(FDieSignature, class AMyCharacter*);

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UReportDeath : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MYGAME_API IReportDeath
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual FDieSignature& GetReportDeathDelegate() = 0;


};
