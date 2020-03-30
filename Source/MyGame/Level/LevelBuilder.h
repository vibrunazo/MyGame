// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelBuilder.generated.h"

UENUM(BlueprintType)
enum class EWallPos : uint8
{
	Left,
	Right,
	Bottom,
	Top
};

UCLASS()
class MYGAME_API ALevelBuilder : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALevelBuilder();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="LevelBuilder")
	class ULevelStreaming* OnBPCreateLevelByName(FName LevelName);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = LevelBuilder)
	class UBillboardComponent* BBComp;
	TArray<FAssetData> AssetDataList;
	TArray<class URoomDataAsset*> RoomList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyLibrary)
	class UStaticMesh* WallMesh;

private:
	void GenerateLevels();
	class ULevelStreaming* GenerateRoom(FTransform Where);
	void SetAssetListFromRegistry();
	void SetupExternalReferences();
	class URoomDataAsset* GetRandomRoom();
	class AStaticMeshActor* GenerateWall(FTransform Where);
	class AStaticMeshActor* GenerateWall(FTransform Where, EWallPos Pos);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


};
