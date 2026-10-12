#include "FloorLayout.h"
#include "TileTemplate.h"

namespace
{
	// Left, Right, Bottom, Top: the order neighbors are tried in (it decides where the treasure room goes)
	const FIntPoint Directions[] = { FIntPoint(0, -1), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(1, 0) };

	struct FLayoutBuild
	{
		const TArray<UTileTemplate*>& Catalogue;
		FRandomStream& Stream;
		FFloorLayout Layout;

		bool IsFree(FIntPoint Coord) const { return !Layout.Find(Coord); }

		// Tiles of a type, and of a difficulty unless it's negative, and doored or not unless IsDoored is negative
		TArray<UTileTemplate*> TemplatesOfType(ETileType Type, int32 Difficulty = -1, int8 IsDoored = -1) const
		{
			return Catalogue.FilterByPredicate([&](const UTileTemplate* Room)
			{
				return Room && Room->TileType == Type && (Difficulty < 0 || Room->Difficulty == Difficulty)
					&& (IsDoored < 0 || Room->bIsDoored == (IsDoored != 0));
			});
		}

		// A random room of that kind; no draw from the stream when there's none
		UTileTemplate* PickTemplate(ETileType Type, int32 Difficulty, int8 IsDoored = -1) const
		{
			const TArray<UTileTemplate*> Tiles = TemplatesOfType(Type, Difficulty, IsDoored);
			if (Tiles.Num() == 0)
			{
				UE_LOG(LogTemp, Error, TEXT("No room in the catalogue for type %d, difficulty %d, doored %d"), (int32)Type, Difficulty, IsDoored);
				return nullptr;
			}
			return Tiles[Stream.RandRange(0, Tiles.Num() - 1)];
		}

		bool HasNeighborOfType(FIntPoint Coord, ETileType Type) const
		{
			for (const FIntPoint& Dir : Directions)
			{
				const FPlannedTile* Neighbor = Layout.Find(Coord + Dir);
				if (Neighbor && Neighbor->Template && Neighbor->Template->TileType == Type) return true;
			}
			return false;
		}

		// The walk: start room, fight rooms getting harder, a doored room every five, the boss last. It never goes
		// left, so going right is always free
		void Walk(const FFloorLayoutParams& Params)
		{
			int32 X = 0, Y = 0, Difficulty = 0, ChanceOfGoingRight = Params.InitialChanceOfGoingRight;
			int32 DooredIndex = 0;
			for (int32 i = 0; i < Params.NumTiles; i++)
			{
				if (i > 1) Difficulty = 1;
				if (i > 4) Difficulty = 2;
				if (i > 6) Difficulty = 3;
				if (i == Params.NumTiles - 2) ChanceOfGoingRight = 100;
				if (i == Params.NumTiles - 1) Difficulty = 9;
				if (Params.NumTiles > 3 && i % 5 == 0)
				{
					DooredIndex = FMath::Clamp(Stream.RandRange(0, 4) + (i / 5) * 5, 2, Params.NumTiles - 2);
				}
				FPlannedTile Planned;
				Planned.Coord = FIntPoint(X, Y);
				if (i == 0) Planned.Template = PickTemplate(ETileType::Empty, Difficulty);
				else if (i == Params.NumTiles - 1) Planned.Template = PickTemplate(ETileType::Boss, Difficulty);
				else if (DooredIndex > 0 && i == DooredIndex)
				{
					Planned.Template = PickTemplate(ETileType::Enemies, Difficulty, 1);
					Planned.bMinibossReward = true;
				}
				else Planned.Template = PickTemplate(ETileType::Enemies, Difficulty, 0);
				Layout.Tiles.Add(Planned);

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
		void AddTreasure(const FFloorLayoutParams& Params)
		{
			if (Params.NumTiles < 3) return;
			const TArray<UTileTemplate*> Treasures = TemplatesOfType(ETileType::Treasure);
			const int32 WalkLength = Layout.Tiles.Num();
			for (int32 Try = 0; Try < 20; Try++)
			{
				const FIntPoint Next = Layout.Tiles[Stream.RandRange(2, WalkLength - 2)].Coord;
				if (Treasures.Num() == 0) continue;
				const FIntPoint* Free = nullptr;
				for (const FIntPoint& Dir : Directions)
				{
					if (IsFree(Next + Dir)) { Free = &Dir; break; }
				}
				if (!Free || HasNeighborOfType(Next + *Free, ETileType::Boss)) continue;
				FPlannedTile Treasure;
				Treasure.Coord = Next + *Free;
				Treasure.Template = Treasures[0];
				Layout.Tiles.Add(Treasure);
				return;
			}
		}
	};
}

FFloorLayout FFloorLayoutGenerator::Generate(const FFloorLayoutParams& Params, const TArray<UTileTemplate*>& Catalogue, FRandomStream& Stream)
{
	FLayoutBuild Build{ Catalogue, Stream, {} };
	Build.Walk(Params);
	Build.AddTreasure(Params);
	return MoveTemp(Build.Layout);
}
