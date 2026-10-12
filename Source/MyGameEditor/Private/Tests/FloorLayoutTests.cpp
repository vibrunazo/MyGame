// FFloorLayoutGenerator rules, checked over many seeds with a small synthetic room catalogue.
// Run: UnrealEditor-Cmd MyGame.uproject -ExecCmds="Automation RunTests MyGame.Layout; Quit" -unattended -nullrhi

#include "Misc/AutomationTest.h"
#include "Level/FloorLayout.h"
#include "Level/TileTemplate.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UTileTemplate* MakeTemplate(ETileType Type, int32 Difficulty, bool bDoored)
	{
		UTileTemplate* Room = NewObject<UTileTemplate>(GetTransientPackage());
		Room->TileType = Type;
		Room->Difficulty = Difficulty;
		Room->bIsDoored = bDoored;
		Room->bIsWalled = bDoored;
		Room->AddToRoot();
		return Room;
	}

	// One start room, plain and doored fight rooms at every difficulty, a boss and a treasure room
	TArray<UTileTemplate*> MakeCatalogue()
	{
		TArray<UTileTemplate*> Catalogue = { MakeTemplate(ETileType::Empty, 0, false), MakeTemplate(ETileType::Boss, 9, true), MakeTemplate(ETileType::Treasure, 0, false) };
		for (int32 Difficulty = 0; Difficulty <= 3; Difficulty++)
		{
			Catalogue.Add(MakeTemplate(ETileType::Enemies, Difficulty, false));
			Catalogue.Add(MakeTemplate(ETileType::Enemies, Difficulty, true));
		}
		return Catalogue;
	}

	void ReleaseCatalogue(const TArray<UTileTemplate*>& Catalogue)
	{
		for (UTileTemplate* Room : Catalogue) Room->RemoveFromRoot();
	}

	bool Adjacent(FIntPoint A, FIntPoint B) { return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y) == 1; }

	FFloorLayout Generate(int32 Seed, int32 NumTiles, const TArray<UTileTemplate*>& Catalogue)
	{
		FFloorLayoutParams Params;
		Params.NumTiles = NumTiles;
		FRandomStream Stream(Seed);
		return FFloorLayoutGenerator::Generate(Params, Catalogue, Stream);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFloorLayoutDeterminismTest, "MyGame.Layout.Determinism", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFloorLayoutDeterminismTest::RunTest(const FString& Parameters)
{
	const TArray<UTileTemplate*> Catalogue = MakeCatalogue();
	for (int32 Seed = 1; Seed <= 500; Seed++)
	{
		const FFloorLayout A = Generate(Seed, 7, Catalogue), B = Generate(Seed, 7, Catalogue);
		bool bSame = A.Tiles.Num() == B.Tiles.Num();
		for (int32 i = 0; bSame && i < A.Tiles.Num(); i++)
		{
			bSame = A.Tiles[i].Coord == B.Tiles[i].Coord && A.Tiles[i].Template == B.Tiles[i].Template && A.Tiles[i].bMinibossReward == B.Tiles[i].bMinibossReward;
		}
		if (!bSame) AddError(FString::Printf(TEXT("seed %d gave two different layouts"), Seed));
	}
	ReleaseCatalogue(Catalogue);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFloorLayoutRulesTest, "MyGame.Layout.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFloorLayoutRulesTest::RunTest(const FString& Parameters)
{
	const TArray<UTileTemplate*> Catalogue = MakeCatalogue();
	int32 Failures = 0, WithTreasure = 0, Runs = 0;
	auto Fail = [&](int32 Seed, int32 NumTiles, const TCHAR* What)
	{
		if (Failures++ < 20) AddError(FString::Printf(TEXT("seed %d, %d rooms: %s"), Seed, NumTiles, What));
	};
	for (int32 NumTiles = 4; NumTiles <= 13; NumTiles++)
	{
		for (int32 Seed = 1; Seed <= 1000; Seed++, Runs++)
		{
			const FFloorLayout Layout = Generate(Seed, NumTiles, Catalogue);
			const TArray<FPlannedTile>& Tiles = Layout.Tiles;
			if (Tiles.Num() < NumTiles || Tiles.Num() > NumTiles + 1) { Fail(Seed, NumTiles, TEXT("wrong room count")); continue; }
			TSet<FIntPoint> Cells;
			for (const FPlannedTile& R : Tiles) Cells.Add(R.Coord);
			if (Cells.Num() != Tiles.Num()) Fail(Seed, NumTiles, TEXT("two rooms on one cell"));
			if (Tiles[0].Coord != FIntPoint::ZeroValue || !Tiles[0].Template || Tiles[0].Template->TileType != ETileType::Empty) Fail(Seed, NumTiles, TEXT("the start isn't an empty tile at 0,0"));
			const FPlannedTile& Boss = Tiles[NumTiles - 1];
			if (!Boss.Template || Boss.Template->TileType != ETileType::Boss) Fail(Seed, NumTiles, TEXT("the walk doesn't end in the boss tile"));
			for (int32 i = 1; i < NumTiles; i++)
			{
				if (!Adjacent(Tiles[i].Coord, Tiles[i - 1].Coord)) Fail(Seed, NumTiles, TEXT("the walk jumps"));
				if (Tiles[i].Coord.Y < Tiles[i - 1].Coord.Y) Fail(Seed, NumTiles, TEXT("the walk went left"));
				if (i < NumTiles - 1 && (!Tiles[i].Template || Tiles[i].Template->TileType != ETileType::Enemies)) Fail(Seed, NumTiles, TEXT("a middle tile isn't a fight tile"));
			}
			for (int32 i = 0; i < NumTiles; i++)
			{
				if (!Tiles[i].bMinibossReward) continue;
				if (i < 2 || i > NumTiles - 2) Fail(Seed, NumTiles, TEXT("doored room too close to the start or the end"));
				if (!Tiles[i].Template || !Tiles[i].Template->bIsDoored) Fail(Seed, NumTiles, TEXT("the miniboss room isn't doored"));
				for (int32 j = i + 1; j < NumTiles; j++)
				{
					if (Tiles[j].bMinibossReward && j / 5 == i / 5) Fail(Seed, NumTiles, TEXT("two doored rooms in one block of five"));
				}
			}
			if (Tiles.Num() == NumTiles + 1)
			{
				WithTreasure++;
				const FPlannedTile& Treasure = Tiles.Last();
				if (!Treasure.Template || Treasure.Template->TileType != ETileType::Treasure) Fail(Seed, NumTiles, TEXT("the extra tile isn't the treasure tile"));
				if (Adjacent(Treasure.Coord, Boss.Coord)) Fail(Seed, NumTiles, TEXT("treasure next to the boss"));
				bool bTouchesWalk = false;
				for (int32 i = 0; i < NumTiles; i++) bTouchesWalk |= Adjacent(Treasure.Coord, Tiles[i].Coord);
				if (!bTouchesWalk) Fail(Seed, NumTiles, TEXT("the treasure room doesn't touch the walk"));
			}
		}
	}
	AddInfo(FString::Printf(TEXT("%d layouts, %d with a treasure room, %d rule breaks"), Runs, WithTreasure, Failures));
	ReleaseCatalogue(Catalogue);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFloorLayoutEmptyCatalogueTest, "MyGame.Layout.EmptyCatalogue", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFloorLayoutEmptyCatalogueTest::RunTest(const FString& Parameters)
{
	AddExpectedError(TEXT("No room in the catalogue"), EAutomationExpectedErrorFlags::Contains, 0);
	const FFloorLayout Layout = Generate(42, 7, {});
	TestEqual(TEXT("the walk still has its cells"), Layout.Tiles.Num(), 7);
	for (const FPlannedTile& R : Layout.Tiles) TestNull(TEXT("no room without a catalogue"), R.Template.Get());
	return true;
}

#endif
