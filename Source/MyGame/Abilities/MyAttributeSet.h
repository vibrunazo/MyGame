// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.generated.h"

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) 	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) 	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) 	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) 	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * MyGame's own attributes beside the plugin's set (UMyCombatAttributes, Ass Step 11): Rot Speed, a multiplier of a
 * character's walk and run rotation rates.
 */
UCLASS()
class MYGAME_API UMyAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMyAttributeSet();

	UPROPERTY(Category = "Attributes | RotSpeed", EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData RotSpeed;
	ATTRIBUTE_ACCESSORS(UMyAttributeSet, RotSpeed)
};
