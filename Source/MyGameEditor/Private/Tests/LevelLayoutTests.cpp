// FLevelLayoutGenerator rules, checked over many seeds with a small synthetic room catalogue.
// Run: UnrealEditor-Cmd MyGame.uproject -ExecCmds="Automation RunTests MyGame.Layout; Quit" -unattended -nullrhi

#include "Misc/AutomationTest.h"
#include "Level/LevelLayout.h"
#include "Level/RoomDataAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	URoomDataAsset* MakeRoom(ERoomType Type, int32 Difficulty, bool bDoored)
	{
		URoomDataAsset* Room = NewObject<URoomDataAsset>(GetTransientPackage());
		Room->RoomType = Type;
		Room->RoomDifficulty = Difficulty;
		Room->bIsDoored = bDoored;
		Room->bIsWalled = bDoored;
		Room->AddToRoot();
		return Room;
	}

	// One start room, plain and doored fight rooms at every difficulty, a boss and a treasure room
	TArray<URoomDataAsset*> MakeCatalogue()
	{
		TArray<URoomDataAsset*> Catalogue = { MakeRoom(ERoomType::Empty, 0, false), MakeRoom(ERoomType::Boss, 9, true), MakeRoom(ERoomType::Treasure, 0, false) };
		for (int32 Difficulty = 0; Difficulty <= 3; Difficulty++)
		{
			Catalogue.Add(MakeRoom(ERoomType::Enemies, Difficulty, false));
			Catalogue.Add(MakeRoom(ERoomType::Enemies, Difficulty, true));
		}
		return Catalogue;
	}

	void ReleaseCatalogue(const TArray<URoomDataAsset*>& Catalogue)
	{
		for (URoomDataAsset* Room : Catalogue) Room->RemoveFromRoot();
	}

	bool Adjacent(FIntPoint A, FIntPoint B) { return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y) == 1; }

	FLevelLayout Generate(int32 Seed, int32 NumRooms, const TArray<URoomDataAsset*>& Catalogue)
	{
		FLevelLayoutParams Params;
		Params.NumRooms = NumRooms;
		FRandomStream Stream(Seed);
		return FLevelLayoutGenerator::Generate(Params, Catalogue, Stream);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelLayoutDeterminismTest, "MyGame.Layout.Determinism", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FLevelLayoutDeterminismTest::RunTest(const FString& Parameters)
{
	const TArray<URoomDataAsset*> Catalogue = MakeCatalogue();
	for (int32 Seed = 1; Seed <= 500; Seed++)
	{
		const FLevelLayout A = Generate(Seed, 7, Catalogue), B = Generate(Seed, 7, Catalogue);
		bool bSame = A.Rooms.Num() == B.Rooms.Num();
		for (int32 i = 0; bSame && i < A.Rooms.Num(); i++)
		{
			bSame = A.Rooms[i].Coord == B.Rooms[i].Coord && A.Rooms[i].Room == B.Rooms[i].Room && A.Rooms[i].bMinibossReward == B.Rooms[i].bMinibossReward;
		}
		if (!bSame) AddError(FString::Printf(TEXT("seed %d gave two different layouts"), Seed));
	}
	ReleaseCatalogue(Catalogue);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelLayoutRulesTest, "MyGame.Layout.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FLevelLayoutRulesTest::RunTest(const FString& Parameters)
{
	const TArray<URoomDataAsset*> Catalogue = MakeCatalogue();
	int32 Failures = 0, WithTreasure = 0, Runs = 0;
	auto Fail = [&](int32 Seed, int32 NumRooms, const TCHAR* What)
	{
		if (Failures++ < 20) AddError(FString::Printf(TEXT("seed %d, %d rooms: %s"), Seed, NumRooms, What));
	};
	for (int32 NumRooms = 4; NumRooms <= 13; NumRooms++)
	{
		for (int32 Seed = 1; Seed <= 1000; Seed++, Runs++)
		{
			const FLevelLayout Layout = Generate(Seed, NumRooms, Catalogue);
			const TArray<FPlannedRoom>& Rooms = Layout.Rooms;
			if (Rooms.Num() < NumRooms || Rooms.Num() > NumRooms + 1) { Fail(Seed, NumRooms, TEXT("wrong room count")); continue; }
			TSet<FIntPoint> Cells;
			for (const FPlannedRoom& R : Rooms) Cells.Add(R.Coord);
			if (Cells.Num() != Rooms.Num()) Fail(Seed, NumRooms, TEXT("two rooms on one cell"));
			if (Rooms[0].Coord != FIntPoint::ZeroValue || !Rooms[0].Room || Rooms[0].Room->RoomType != ERoomType::Empty) Fail(Seed, NumRooms, TEXT("the start isn't an empty room at 0,0"));
			const FPlannedRoom& Boss = Rooms[NumRooms - 1];
			if (!Boss.Room || Boss.Room->RoomType != ERoomType::Boss) Fail(Seed, NumRooms, TEXT("the walk doesn't end in the boss room"));
			for (int32 i = 1; i < NumRooms; i++)
			{
				if (!Adjacent(Rooms[i].Coord, Rooms[i - 1].Coord)) Fail(Seed, NumRooms, TEXT("the walk jumps"));
				if (Rooms[i].Coord.Y < Rooms[i - 1].Coord.Y) Fail(Seed, NumRooms, TEXT("the walk went left"));
				if (i < NumRooms - 1 && (!Rooms[i].Room || Rooms[i].Room->RoomType != ERoomType::Enemies)) Fail(Seed, NumRooms, TEXT("a middle room isn't a fight room"));
			}
			for (int32 i = 0; i < NumRooms; i++)
			{
				if (!Rooms[i].bMinibossReward) continue;
				if (i < 2 || i > NumRooms - 2) Fail(Seed, NumRooms, TEXT("doored room too close to the start or the end"));
				if (!Rooms[i].Room || !Rooms[i].Room->bIsDoored) Fail(Seed, NumRooms, TEXT("the miniboss room isn't doored"));
				for (int32 j = i + 1; j < NumRooms; j++)
				{
					if (Rooms[j].bMinibossReward && j / 5 == i / 5) Fail(Seed, NumRooms, TEXT("two doored rooms in one block of five"));
				}
			}
			if (Rooms.Num() == NumRooms + 1)
			{
				WithTreasure++;
				const FPlannedRoom& Treasure = Rooms.Last();
				if (!Treasure.Room || Treasure.Room->RoomType != ERoomType::Treasure) Fail(Seed, NumRooms, TEXT("the extra room isn't the treasure room"));
				if (Adjacent(Treasure.Coord, Boss.Coord)) Fail(Seed, NumRooms, TEXT("treasure next to the boss"));
				bool bTouchesWalk = false;
				for (int32 i = 0; i < NumRooms; i++) bTouchesWalk |= Adjacent(Treasure.Coord, Rooms[i].Coord);
				if (!bTouchesWalk) Fail(Seed, NumRooms, TEXT("the treasure room doesn't touch the walk"));
			}
		}
	}
	AddInfo(FString::Printf(TEXT("%d layouts, %d with a treasure room, %d rule breaks"), Runs, WithTreasure, Failures));
	ReleaseCatalogue(Catalogue);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelLayoutEmptyCatalogueTest, "MyGame.Layout.EmptyCatalogue", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FLevelLayoutEmptyCatalogueTest::RunTest(const FString& Parameters)
{
	AddExpectedError(TEXT("No room in the catalogue"), EAutomationExpectedErrorFlags::Contains, 0);
	const FLevelLayout Layout = Generate(42, 7, {});
	TestEqual(TEXT("the walk still has its cells"), Layout.Rooms.Num(), 7);
	for (const FPlannedRoom& R : Layout.Rooms) TestNull(TEXT("no room without a catalogue"), R.Room.Get());
	return true;
}

#endif
