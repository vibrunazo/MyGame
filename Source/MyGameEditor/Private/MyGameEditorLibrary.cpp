#include "MyGameEditorLibrary.h"
#include "Animation/AnimMontage.h"
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
	GetObjectsWithOuter(Object, Inner, false);
	for (UObject* Obj : Inner)
	{
		if (Obj) Out.Add(Obj->GetPathName());
	}
	Out.Sort();
	return Out;
}
