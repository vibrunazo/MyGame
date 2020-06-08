// Fill out your copyright notice in the Description page of Project Settings.


#include "HitboxSettings.h"

UHitboxesContainer::UHitboxesContainer()
{

}

UHitboxesContainer::UHitboxesContainer(TArray<FHitboxSettings> NewHitboxes)
{
	Hitboxes = NewHitboxes;
}