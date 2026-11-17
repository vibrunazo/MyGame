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
