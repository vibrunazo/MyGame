#include "LevelLayout.h"
#include "RoomDataAsset.h"

namespace
{
	// Left, Right, Bottom, Top: the order neighbors are tried in (it decides where the treasure room goes)
	const FIntPoint Directions[] = { FIntPoint(0, -1), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(1, 0) };

	struct FLayoutBuild
	{
		const TArray<URoomDataAsset*>& Catalogue;
		FRandomStream& Stream;
		FLevelLayout Layout;

		bool IsFree(FIntPoint Coord) const { return !Layout.Find(Coord); }

		// Rooms of a type, and of a difficulty unless it's negative, and doored or not unless IsDoored is negative
		TArray<URoomDataAsset*> RoomsOfType(ERoomType Type, int32 Difficulty = -1, int8 IsDoored = -1) const
		{
			return Catalogue.FilterByPredicate([&](const URoomDataAsset* Room)
			{
				return Room && Room->RoomType == Type && (Difficulty < 0 || Room->RoomDifficulty == Difficulty)
					&& (IsDoored < 0 || Room->bIsDoored == (IsDoored != 0));
			});
		}

		// A random room of that kind; no draw from the stream when there's none
		URoomDataAsset* PickRoom(ERoomType Type, int32 Difficulty, int8 IsDoored = -1) const
		{
			const TArray<URoomDataAsset*> Rooms = RoomsOfType(Type, Difficulty, IsDoored);
			if (Rooms.Num() == 0)
			{
				UE_LOG(LogTemp, Error, TEXT("No room in the catalogue for type %d, difficulty %d, doored %d"), (int32)Type, Difficulty, IsDoored);
				return nullptr;
			}
			return Rooms[Stream.RandRange(0, Rooms.Num() - 1)];
		}

		bool HasNeighborOfType(FIntPoint Coord, ERoomType Type) const
		{
			for (const FIntPoint& Dir : Directions)
			{
				const FPlannedRoom* Neighbor = Layout.Find(Coord + Dir);
				if (Neighbor && Neighbor->Room && Neighbor->Room->RoomType == Type) return true;
			}
			return false;
		}

		// The walk: start room, fight rooms getting harder, a doored room every five, the boss last. It never goes
		// left, so going right is always free
		void Walk(const FLevelLayoutParams& Params)
		{
			int32 X = 0, Y = 0, Difficulty = 0, ChanceOfGoingRight = Params.InitialChanceOfGoingRight;
			int32 DooredRoom = 0;
			for (int32 i = 0; i < Params.NumRooms; i++)
			{
				if (i > 1) Difficulty = 1;
				if (i > 4) Difficulty = 2;
				if (i > 6) Difficulty = 3;
				if (i == Params.NumRooms - 2) ChanceOfGoingRight = 100;
				if (i == Params.NumRooms - 1) Difficulty = 9;
				if (Params.NumRooms > 3 && i % 5 == 0)
				{
					DooredRoom = FMath::Clamp(Stream.RandRange(0, 4) + (i / 5) * 5, 2, Params.NumRooms - 2);
				}
				FPlannedRoom Planned;
				Planned.Coord = FIntPoint(X, Y);
				if (i == 0) Planned.Room = PickRoom(ERoomType::Empty, Difficulty);
				else if (i == Params.NumRooms - 1) Planned.Room = PickRoom(ERoomType::Boss, Difficulty);
				else if (DooredRoom > 0 && i == DooredRoom)
				{
					Planned.Room = PickRoom(ERoomType::Enemies, Difficulty, 1);
					Planned.bMinibossReward = true;
				}
				else Planned.Room = PickRoom(ERoomType::Enemies, Difficulty, 0);
				Layout.Rooms.Add(Planned);

				if (Stream.RandRange(1, 100) <= ChanceOfGoingRight)
				{
					Y++;
					ChanceOfGoingRight -= Params.DecWhenChoseRight;
				}
				else
				{
					ChanceOfGoingRight += Params.IncWhenChoseVert;
					// the random vertical side, then the other one, else right
					const bool bTopFirst = Stream.RandRange(0, 1) != 0;
					const int32 First = bTopFirst ? 1 : -1;
					if (IsFree(FIntPoint(X + First, Y))) X += First;
					else if (IsFree(FIntPoint(X - First, Y))) X -= First;
					else Y++;
				}
			}
		}

		// The treasure room: next to a random room between the second and the one before the boss, on its first free
		// side, never next to the boss. Up to 20 tries; the level can end up without one
		void AddTreasure(const FLevelLayoutParams& Params)
		{
			if (Params.NumRooms < 3) return;
			const TArray<URoomDataAsset*> Treasures = RoomsOfType(ERoomType::Treasure);
			const int32 WalkLength = Layout.Rooms.Num();
			for (int32 Try = 0; Try < 20; Try++)
			{
				const FIntPoint Next = Layout.Rooms[Stream.RandRange(2, WalkLength - 2)].Coord;
				if (Treasures.Num() == 0) continue;
				const FIntPoint* Free = nullptr;
				for (const FIntPoint& Dir : Directions)
				{
					if (IsFree(Next + Dir)) { Free = &Dir; break; }
				}
				if (!Free || HasNeighborOfType(Next + *Free, ERoomType::Boss)) continue;
				FPlannedRoom Treasure;
				Treasure.Coord = Next + *Free;
				Treasure.Room = Treasures[0];
				Layout.Rooms.Add(Treasure);
				return;
			}
		}
	};
}

FLevelLayout FLevelLayoutGenerator::Generate(const FLevelLayoutParams& Params, const TArray<URoomDataAsset*>& Catalogue, FRandomStream& Stream)
{
	FLayoutBuild Build{ Catalogue, Stream, {} };
	Build.Walk(Params);
	Build.AddTreasure(Params);
	return MoveTemp(Build.Layout);
}
