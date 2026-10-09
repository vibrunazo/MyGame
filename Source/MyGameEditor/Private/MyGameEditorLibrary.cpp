#include "MyGameEditorLibrary.h"

#include "Sound/SoundCue.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundWave.h"
#include "GameplayEffect.h"
#include "Abilities/MyGameplayEffectUIData.h"

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

int32 UMyGameEditorLibrary::ReplaceBlueprintUIDataWithNative(TSubclassOf<UGameplayEffect> EffectClass)
{
	UGameplayEffect* Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	FArrayProperty* Prop = FindFProperty<FArrayProperty>(UGameplayEffect::StaticClass(), TEXT("GEComponents"));
	if (!Effect || !Prop) return 0;
	// GEComponents is protected; the array is reached through reflection like the details panel does
	TArray<TObjectPtr<UGameplayEffectComponent>>& Components = *Prop->ContainerPtrToValuePtr<TArray<TObjectPtr<UGameplayEffectComponent>>>(Effect);
	int32 Replaced = 0;
	Effect->Modify();
	for (TObjectPtr<UGameplayEffectComponent>& Component : Components)
	{
		const UMyGameplayEffectUIData* Old = Cast<UMyGameplayEffectUIData>(Component);
		if (!Old || Old->GetClass() == UMyGameplayEffectUIData::StaticClass()) continue;
		UMyGameplayEffectUIData* Native = NewObject<UMyGameplayEffectUIData>(Effect, NAME_None, Effect->GetMaskedFlags(RF_PropagateToSubObjects) | RF_Transactional);
		Native->BuffUI = Old->BuffUI;
		Component = Native;
		++Replaced;
	}
	if (Replaced)
	{
		Effect->PostEditChange();
		Effect->MarkPackageDirty();
	}
	return Replaced;
}
