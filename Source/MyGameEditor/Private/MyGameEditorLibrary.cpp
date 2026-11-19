#include "MyGameEditorLibrary.h"
#include "CombatRecorder.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Engine/Engine.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/Skeleton.h"
#include "AssHitBox.h"
#include "AssMoverComponent.h"
#include "AssSpots.h"
#include "AssTokenArbiterComponent.h"
#include "AIController.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "GameFramework/Character.h"
#include "EngineUtils.h"
#include "Player/MyCharacter.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "Sound/SoundCue.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundWave.h"

bool UMyGameEditorLibrary::RebuildSoundCueFromWaves(USoundCue* Cue, const TArray<USoundWave*>& Waves, bool bLooping)
{
	if (!Cue || Waves.IsEmpty() || Waves.Contains(nullptr))
	{
		UE_LOG(LogTemp, Error, TEXT("RebuildSoundCueFromWaves: need a cue and at least one valid wave"));
		return false;
	}

	Cue->Modify();
	Cue->AllNodes.Empty();
	Cue->ClearGraph();
	Cue->FirstNode = nullptr;

	TArray<USoundNodeWavePlayer*> Players;
	for (USoundWave* Wave : Waves)
	{
		USoundNodeWavePlayer* Player = Cue->ConstructSoundNode<USoundNodeWavePlayer>(USoundNodeWavePlayer::StaticClass(), false);
		Player->SetSoundWave(Wave);
		Player->bLooping = bLooping;
		Players.Add(Player);
	}

	if (Players.Num() == 1)
	{
		Cue->FirstNode = Players[0];
	}
	else
	{
		USoundNodeRandom* Random = Cue->ConstructSoundNode<USoundNodeRandom>(USoundNodeRandom::StaticClass(), false);
		for (USoundNodeWavePlayer* Player : Players)
		{
			// InsertChildNode keeps Random's Weights array in step with its children
			Random->InsertChildNode(Random->ChildNodes.Num());
			Random->ChildNodes.Last() = Player;
		}
		Cue->FirstNode = Random;
	}

	Cue->LinkGraphNodesFromSoundNodes();
	Cue->PostEditChange();
	Cue->MarkPackageDirty();
	return true;
}

FString UMyGameEditorLibrary::DescribeMontageSections(UAnimMontage* Montage)
{
	TArray<TSharedPtr<FJsonValue>> Sections;
	if (Montage)
	{
		for (const FCompositeSection& Section : Montage->CompositeSections)
		{
			TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetStringField(TEXT("name"), Section.SectionName.ToString());
			Obj->SetNumberField(TEXT("start"), Section.GetTime());
			Obj->SetStringField(TEXT("next"), Section.NextSectionName.ToString());
			Sections.Add(MakeShared<FJsonValueObject>(Obj));
		}
	}
	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Sections, Writer);
	return Out;
}

TArray<FString> UMyGameEditorLibrary::ListSubobjects(UObject* Object)
{
	TArray<FString> Out;
	if (!Object) return Out;
	TArray<UObject*> Inner;
	GetObjectsWithOuter(Object, Inner, EGetObjectsFlags::None);
	for (UObject* Obj : Inner)
	{
		if (Obj) Out.Add(Obj->GetPathName());
	}
	Out.Sort();
	return Out;
}

bool UMyGameEditorLibrary::StartCombatRecord(UObject* WorldContext, const TArray<FString>& Roles, const TArray<AActor*>& Actors)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	return UCombatRecorder::Start(World, Roles, Actors);
}

FString UMyGameEditorLibrary::StopCombatRecord()
{
	return UCombatRecorder::Stop();
}

FGameplayTagContainer UMyGameEditorLibrary::SetEffectGrantedTagsForTest(TSubclassOf<UGameplayEffect> Effect, const FGameplayTagContainer& Added)
{
	UGameplayEffect* Default = Effect ? Effect->GetDefaultObject<UGameplayEffect>() : nullptr;
	UTargetTagsGameplayEffectComponent* Tags = Default ? const_cast<UTargetTagsGameplayEffectComponent*>(Default->FindComponent<UTargetTagsGameplayEffectComponent>()) : nullptr;
	if (!Tags) return FGameplayTagContainer();
	FInheritedTagContainer Changes = Tags->GetConfiguredTargetTagChanges();
	const FGameplayTagContainer Was = Changes.Added;
	// the effect's cached tags only gain what Added holds and lose what Removed holds
	Changes.Removed = Was;
	Changes.Removed.RemoveTags(Added);
	Changes.Added = Added;
	Tags->SetAndApplyTargetTagChanges(Changes);
	return Was;
}

FString UMyGameEditorLibrary::GetPropertyTextForTest(UObject* Object, FName Property)
{
	const FProperty* Prop = Object ? Object->GetClass()->FindPropertyByName(Property) : nullptr;
	if (!Prop) return FString();
	FString Text;
	Prop->ExportTextItem_Direct(Text, Prop->ContainerPtrToValuePtr<void>(Object), nullptr, Object, PPF_None);
	return Text;
}

bool UMyGameEditorLibrary::SetPropertyTextForTest(UObject* Object, FName Property, const FString& Text)
{
	const FProperty* Prop = Object ? Object->GetClass()->FindPropertyByName(Property) : nullptr;
	if (!Prop) return false;
	return Prop->ImportText_Direct(*Text, Prop->ContainerPtrToValuePtr<void>(Object), Object, PPF_None) != nullptr;
}

void UMyGameEditorLibrary::QueryCombatRecordRoles(const TArray<FString>& Roles)
{
	UCombatRecorder::QueryRoles(Roles);
}

bool UMyGameEditorLibrary::MoveAnimationToSkeleton(UAnimationAsset* Asset, USkeletalMesh* Mesh, UAnimSequence* Retargeted)
{
	USkeleton* Skeleton = Mesh ? Mesh->GetSkeleton() : nullptr;
	if (!Asset || !Skeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("MoveAnimationToSkeleton: need an asset and a mesh with a skeleton"));
		return false;
	}
	UAnimSequence* Sequence = Cast<UAnimSequence>(Asset);
	if (Sequence && (!Retargeted || Retargeted->GetSkeleton() != Skeleton))
	{
		UE_LOG(LogTemp, Error, TEXT("MoveAnimationToSkeleton: %s needs its retargeted copy on %s"), *Asset->GetPathName(), *Skeleton->GetPathName());
		return false;
	}
	Asset->Modify();
	Asset->SetSkeleton(Skeleton);
	Asset->SetPreviewMesh(Mesh);
	if (Sequence)
	{
		// what the IK retargeter's batch operation does to its copy, done to the original from the copy's data
		const IAnimationDataModel* From = Retargeted->GetDataModel();
		IAnimationDataController& To = Sequence->GetController();
		constexpr bool bTransact = false;
		To.OpenBracket(FText::FromString(TEXT("Move to another skeleton")), bTransact);
		To.RemoveAllCurvesOfType(ERawCurveTrackTypes::RCT_Transform, bTransact);
		To.RemoveAllBoneTracks(bTransact);
		To.UpdateWithSkeleton(Skeleton, bTransact);
		To.SetFrameRate(From->GetFrameRate(), bTransact);
		To.SetNumberOfFrames(From->GetNumberOfFrames(), bTransact);
		TArray<FName> Bones;
		From->GetBoneTrackNames(Bones);
		bool bOk = true;
		for (const FName& Bone : Bones)
		{
			TArray<FTransform> Keys;
			From->GetBoneTrackTransforms(Bone, Keys);
			TArray<FVector> Positions;
			TArray<FQuat> Rotations;
			TArray<FVector> Scales;
			for (const FTransform& Key : Keys)
			{
				Positions.Add(Key.GetLocation());
				Rotations.Add(Key.GetRotation());
				Scales.Add(Key.GetScale3D());
			}
			bOk &= To.AddBoneCurve(Bone, bTransact);
			bOk &= To.SetBoneTrackKeys(Bone, Positions, Rotations, Scales, bTransact);
		}
		To.CloseBracket(bTransact);
		Sequence->RetargetSource = NAME_None;
		Sequence->SetRetargetSourceAsset(Mesh);
		Sequence->UpdateRetargetSourceAssetData();
		if (!bOk)
		{
			UE_LOG(LogTemp, Error, TEXT("MoveAnimationToSkeleton: %s refused a bone track"), *Asset->GetPathName());
			return false;
		}
	}
	Asset->PostEditChange();
	Asset->MarkPackageDirty();
	return true;
}

bool UMyGameEditorLibrary::MoveAnimBlueprintToSkeleton(UAnimBlueprint* Blueprint, USkeletalMesh* Mesh)
{
	USkeleton* Skeleton = Mesh ? Mesh->GetSkeleton() : nullptr;
	if (!Blueprint || !Skeleton)
	{
		return false;
	}
	Blueprint->Modify();
	Blueprint->TargetSkeleton = Skeleton;
	Blueprint->SetPreviewMesh(Mesh);
	FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection);
	Blueprint->PostEditChange();
	Blueprint->MarkPackageDirty();
	return Blueprint->Status != BS_Error;
}

bool UMyGameEditorLibrary::SetSkeletonSocket(USkeleton* Skeleton, FName Socket, FName Bone, const FTransform& Local)
{
	if (!Skeleton || Socket.IsNone() || Skeleton->GetReferenceSkeleton().FindBoneIndex(Bone) == INDEX_NONE)
	{
		return false;
	}
	Skeleton->Modify();
	USkeletalMeshSocket* Found = Skeleton->FindSocket(Socket);
	if (!Found)
	{
		Found = NewObject<USkeletalMeshSocket>(Skeleton);
		Found->SocketName = Socket;
		Skeleton->Sockets.Add(Found);
	}
	Found->BoneName = Bone;
	Found->RelativeLocation = Local.GetLocation();
	Found->RelativeRotation = Local.Rotator();
	Found->RelativeScale = Local.GetScale3D();
	Skeleton->MarkPackageDirty();
	return true;
}

bool UMyGameEditorLibrary::MontageBoneForTest(UAnimMontage* Montage, FName Bone, float Position, FTransform& Transform)
{
	Transform = FTransform::Identity;
	return Montage && AAssHitBox::MontageBoneInComponent(Montage, AAssHitBox::SkeletonChainOf(Montage->GetSkeleton(), Bone), Position, Transform);
}

namespace MyGameWorldChecks
{
	/** A point to the centimetre, to compare two searches' points. */
	FIntVector Cell(const FVector& Point)
	{
		return FIntVector(FMath::RoundToInt(Point.X), FMath::RoundToInt(Point.Y), FMath::RoundToInt(Point.Z));
	}

	TSharedRef<FJsonValue> Row(std::initializer_list<double> Values)
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		for (const double Value : Values) Out.Add(MakeShared<FJsonValueNumber>(Value));
		return MakeShared<FJsonValueArray>(Out);
	}

	// The token shadow: its arbiter, the player it sits on, and what it found
	TWeakObjectPtr<UAssTokenArbiterComponent> ShadowArbiter;
	TWeakObjectPtr<AMyCharacter> ShadowPlayer;
	int32 ShadowQuestions = 0;
	int32 ShadowAdditions = 0;
	TArray<FString> ShadowMismatches;
	/** Each addition: its frame and who. */
	TArray<FString> ShadowAdded;
	/** Answers that differ only because a holder was removed without dying, which the arbiter lets go at once. */
	TArray<FString> ShadowRemoved;
}

FString UMyGameEditorLibrary::CompareSpotSearchForTest(APawn* Querier, ACharacter* Target, UEnvQuery* Query, float Behind,
	float HalfExtent, float Spacing, float DistanceWeight, float SightScore, float Ratio)
{
	using namespace MyGameWorldChecks;
	AMyCharacter* Self = Cast<AMyCharacter>(Querier);
	AAIController* Controller = Querier ? Cast<AAIController>(Querier->GetController()) : nullptr;
	UWorld* World = Querier ? Querier->GetWorld() : nullptr;
	UEnvQueryManager* Queries = World ? UEnvQueryManager::GetCurrent(World) : nullptr;
	if (!Self || !Controller || !Target || !Query || !Queries) return FString();
	// The query's contexts read the querier's target
	Self->SetTargetEnemy(Target);

	// The engine: a copy of the query, its scores left as the best-25% pick sees them
	UEnvQuery* Copy = DuplicateObject<UEnvQuery>(Query, GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UEnvQuery::StaticClass(), TEXT("SpotCheck")));
	for (UEnvQueryOption* Option : Copy->GetOptions())
	{
		UEnvQueryGenerator* Generator = Option ? Option->Generator.Get() : nullptr;
		FProperty* Normalization = Generator ? Generator->GetClass()->FindPropertyByName(TEXT("EnvQueryResultNormalizationOption")) : nullptr;
		if (!Normalization || !Normalization->ImportText_InContainer(TEXT("Unaltered"), Generator, Generator, PPF_None)) return FString();
	}
	const TSharedPtr<FEnvQueryResult> Result = Queries->RunInstantQuery(FEnvQueryRequest(Copy, Querier), EEnvQueryRunMode::AllMatching);
	TArray<TSharedPtr<FJsonValue>> EngineBand, EngineAll;
	TSet<FIntVector> EngineCells;
	const int32 EnginePoints = Result.IsValid() ? Result->Items.Num() : 0;
	for (int32 Index = 0; Index < EnginePoints; ++Index)
	{
		const FVector Point = Result->GetItemAsLocation(Index);
		EngineAll.Add(Row({ Point.X, Point.Y, Point.Z, Result->Items[Index].Score }));
	}
	if (EnginePoints > 0)
	{
		// Sorted best first, as the pick sees them: the band runs to the first item under the share of the best
		const float MinScore = Result->Items[0].Score * Ratio;
		for (int32 Index = 0; Index < EnginePoints && Result->Items[Index].Score >= MinScore; ++Index)
		{
			const FVector Point = Result->GetItemAsLocation(Index);
			EngineBand.Add(Row({ Point.X, Point.Y, Point.Z, Result->Items[Index].Score }));
			EngineCells.Add(Cell(Point));
		}
	}

	// The plugin: its mover's search on the querier's controller, banded as Move To Spot bands it
	UAssMoverComponent* Mover = NewObject<UAssMoverComponent>(Controller);
	Mover->RegisterComponent();
	FAssAICommand Move;
	Move.CommandType = EAssAICommandType::MoveToSpot;
	Move.SpotBehind = Behind;
	Move.SpotHalfExtent = HalfExtent;
	Move.SpotSpacing = Spacing;
	Move.SpotDistanceWeight = DistanceWeight;
	Move.SpotSightScore = SightScore;
	Move.SpotScoreRatio = Ratio;
	TArray<FVector> Points;
	TArray<FAssSpotPoint> Facts;
	Mover->SpotPoints(Target, Move, Points, Facts);
	Mover->DestroyComponent();
	const TArray<float> Scores = AssSpots::Scores(Facts, DistanceWeight, SightScore);
	TArray<TSharedPtr<FJsonValue>> PluginBand, PluginAll;
	TSet<FIntVector> PluginCells;
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		PluginAll.Add(Row({ Points[Index].X, Points[Index].Y, Points[Index].Z, Scores[Index], Facts[Index].bSight ? 1.0 : 0.0, Facts[Index].Distance }));
	}
	for (const int32 Index : AssSpots::Band(Facts, DistanceWeight, SightScore, Ratio))
	{
		const FVector& Point = Points[Index];
		PluginBand.Add(Row({ Point.X, Point.Y, Point.Z, Scores[Index], Facts[Index].bSight ? 1.0 : 0.0, Facts[Index].Distance }));
		PluginCells.Add(Cell(Point));
	}

	const auto Missing = [](const TSet<FIntVector>& From, const TSet<FIntVector>& In)
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		for (const FIntVector& Each : From)
		{
			if (!In.Contains(Each)) Out.Add(Row({ double(Each.X), double(Each.Y), double(Each.Z) }));
		}
		return Out;
	};
	TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetArrayField(TEXT("engine"), EngineBand);
	Out->SetArrayField(TEXT("plugin"), PluginBand);
	Out->SetNumberField(TEXT("engine_points"), EnginePoints);
	Out->SetNumberField(TEXT("plugin_points"), Points.Num());
	Out->SetArrayField(TEXT("only_engine"), Missing(EngineCells, PluginCells));
	Out->SetArrayField(TEXT("only_plugin"), Missing(PluginCells, EngineCells));
	Out->SetArrayField(TEXT("engine_all"), EngineAll);
	Out->SetArrayField(TEXT("plugin_all"), PluginAll);
	FString Text;
	FJsonSerializer::Serialize(Out, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text));
	return Text;
}

bool UMyGameEditorLibrary::RequestTokenForTest(AActor* Target, AActor* Pawn)
{
	UAssTokenArbiterComponent* Arbiter = Target ? Target->FindComponentByClass<UAssTokenArbiterComponent>() : nullptr;
	return Arbiter && Pawn && Arbiter->Request(Pawn);
}

bool UMyGameEditorLibrary::ClearTokensForTest(AActor* Target)
{
	// The player's arbiter (the first: the world check's shadow comes later): every character in the world lets go, as a
	// stopped pattern does
	UAssTokenArbiterComponent* Arbiter = Target ? Target->FindComponentByClass<UAssTokenArbiterComponent>() : nullptr;
	if (!Arbiter) return false;
	for (TActorIterator<ACharacter> It(Target->GetWorld()); It; ++It)
	{
		Arbiter->Release(*It, EAssTokenRelease::Stopped);
	}
	return true;
}

bool UMyGameEditorLibrary::StartTokenShadowForTest(ACharacter* Player)
{
	using namespace MyGameWorldChecks;
	StopTokenShadowForTest();
	AMyCharacter* Owner = Cast<AMyCharacter>(Player);
	if (!Owner) return false;
	UAssTokenArbiterComponent* Arbiter = NewObject<UAssTokenArbiterComponent>(Owner);
	Arbiter->Capacity = 1;
	Arbiter->bKeepUntilDeath = true;
	Arbiter->RegisterComponent();
	ShadowArbiter = Arbiter;
	ShadowPlayer = Owner;
	AMyCharacter::AggroListProbe = [](AMyCharacter* ListOwner, AActor* Asker, bool bAdd, bool bAnswer)
	{
		UAssTokenArbiterComponent* Tokens = ShadowArbiter.Get();
		if (!Tokens || ListOwner != ShadowPlayer.Get()) return;
		const uint64 Frame = GFrameCounter;
		if (bAdd)
		{
			++ShadowAdditions;
			ShadowAdded.Add(FString::Printf(TEXT("frame %llu: %s"), Frame, *GetNameSafe(Asker)));
			if (!Tokens->Request(Asker))
			{
				ShadowMismatches.Add(FString::Printf(TEXT("frame %llu: %s added to the list, refused by the arbiter"), Frame, *GetNameSafe(Asker)));
			}
			return;
		}
		++ShadowQuestions;
		const bool bArbiter = Tokens->HeldOrFree(Asker);
		if (bArbiter != bAnswer)
		{
			// What the list holds, for the reader
			TArray<FString> Entries;
			bool bGoneEntry = false;
			if (const FArrayProperty* List = CastField<FArrayProperty>(ListOwner->GetClass()->FindPropertyByName(TEXT("AggroList"))))
			{
				FScriptArrayHelper Items(List, List->ContainerPtrToValuePtr<void>(ListOwner));
				for (int32 Index = 0; Index < Items.Num(); ++Index)
				{
					const UObject* Entry = CastField<FObjectPropertyBase>(List->Inner)->GetObjectPropertyValue(Items.GetRawPtr(Index));
					bGoneEntry |= !IsValid(Entry);
					Entries.Add(GetNameSafe(Entry) + (IsValid(Entry) ? TEXT("") : TEXT(" (gone)")));
				}
			}
			const FString Line = FString::Printf(TEXT("frame %llu: %s, the list %s [%s], the arbiter %s"), Frame, *GetNameSafe(Asker),
				bAnswer ? TEXT("yes") : TEXT("no"), *FString::Join(Entries, TEXT(", ")), bArbiter ? TEXT("yes") : TEXT("no"));
			// A holder removed without dying: the list keeps it until garbage collection, the arbiter lets it go at once
			// (Plan.md Step 7 item 2, the difference the arbiter is for)
			(bGoneEntry && !bAnswer && bArbiter ? ShadowRemoved : ShadowMismatches).Add(Line);
		}
	};
	return true;
}

FString UMyGameEditorLibrary::StopTokenShadowForTest()
{
	using namespace MyGameWorldChecks;
	AMyCharacter::AggroListProbe = nullptr;
	if (UAssTokenArbiterComponent* Arbiter = ShadowArbiter.Get()) Arbiter->DestroyComponent();
	TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetNumberField(TEXT("questions"), ShadowQuestions);
	Out->SetNumberField(TEXT("additions"), ShadowAdditions);
	TArray<TSharedPtr<FJsonValue>> Lines;
	for (const FString& Line : ShadowMismatches) Lines.Add(MakeShared<FJsonValueString>(Line));
	Out->SetArrayField(TEXT("mismatches"), Lines);
	TArray<TSharedPtr<FJsonValue>> Added;
	for (const FString& Line : ShadowAdded) Added.Add(MakeShared<FJsonValueString>(Line));
	Out->SetArrayField(TEXT("added"), Added);
	TArray<TSharedPtr<FJsonValue>> Removed;
	for (const FString& Line : ShadowRemoved) Removed.Add(MakeShared<FJsonValueString>(Line));
	Out->SetArrayField(TEXT("removed_holder"), Removed);
	ShadowAdded.Reset();
	ShadowRemoved.Reset();
	ShadowArbiter.Reset();
	ShadowPlayer.Reset();
	ShadowQuestions = 0;
	ShadowAdditions = 0;
	ShadowMismatches.Reset();
	FString Text;
	FJsonSerializer::Serialize(Out, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text));
	return Text;
}
