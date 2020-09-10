// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.generated.h"

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 
 */
UCLASS()
class MYGAME_API UMyAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMyAttributeSet();
	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData & Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(Category = "Attributes | Health", EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UMyAttributeSet, Health)

	UPROPERTY(Category = "Attributes | MaxHealth", EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UMyAttributeSet, MaxHealth)

	UPROPERTY(Category = "Attributes | Mana", EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UMyAttributeSet, Mana)

	UPROPERTY(Category = "Attributes | MaxMana", EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UMyAttributeSet, MaxMana)
	
	UPROPERTY(Category = "Attributes | Attack", EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS(UMyAttributeSet, Attack)

	UPROPERTY(Category = "Attributes | Defense", EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UMyAttributeSet, Defense)

	UPROPERTY(Category = "Attributes | Speed", EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData Speed;
	ATTRIBUTE_ACCESSORS(UMyAttributeSet, Speed)

	UPROPERTY(Category = "Attributes | AttackSpeed", EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UMyAttributeSet, AttackSpeed)
};
