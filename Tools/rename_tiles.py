"""One-off naming pass (2026-10-12, CLAUDE.md glossary): tile templates and the Blueprints named after "rooms".

    /Game/Maps/Rooms/RoomXX-YY      -> /Game/Maps/Tiles/TileXX-YY   (and RoomBoss01, RoomTreasure01, the unused Room01/02/03...)
    /Game/Maps/Rooms/DA_RoomXX-YY   -> /Game/Maps/Tiles/DA_TileXX-YY
    /Game/Maps/Rooms/Walls          -> /Game/Maps/Tiles/Walls
    BP_LevelBuilder -> BP_FloorBuilder, BP_RoomMaster -> BP_TileMaster, BP_RoomCameraPawn -> BP_RoomCamera,
    PCG_RoomDressing -> PCG_TileDressing

Renames through AssetTools (references are rewritten), then fixes up and removes the redirectors it left.
Run with the editor closed:
    UnrealEditor-Cmd.exe MyGame.uproject -run=pythonscript -script="<abs path>/Tools/rename_tiles.py" -unattended -nullrhi
"""
import unreal as ue

tools = ue.AssetToolsHelpers.get_asset_tools()
registry = ue.AssetRegistryHelpers.get_asset_registry()

renames = []
for data in registry.get_assets_by_path("/Game/Maps/Rooms", recursive=False):
    name = str(data.asset_name)
    if name.endswith("_BuiltData"):
        continue  # moves with its map
    new = name.replace("DA_Room", "DA_Tile", 1) if name.startswith("DA_Room") else name.replace("Room", "Tile", 1) if name.startswith("Room") else name
    renames.append((data.get_asset(), "/Game/Maps/Tiles", new))
for path, new in [("/Game/Blueprints/Level/BP_LevelBuilder", "BP_FloorBuilder"), ("/Game/Blueprints/Level/BP_RoomMaster", "BP_TileMaster"),
                  ("/Game/Blueprints/BP_RoomCameraPawn", "BP_RoomCamera"), ("/Game/PCG/PCG_RoomDressing", "PCG_TileDressing")]:
    renames.append((ue.load_asset(path), path.rsplit("/", 1)[0], new))

data = [ue.AssetRenameData(asset=a, new_package_path=p, new_name=n) for a, p, n in renames]
ok = tools.rename_assets(data)
ue.log(f"rename_tiles: renamed {len(data)} assets, ok={ok}")

# point every referencer at the new names and remove the redirectors
redirectors = [d.get_asset() for d in registry.get_assets_by_path("/Game", recursive=True)
               if str(d.asset_class_path.asset_name) == "ObjectRedirector"]
if redirectors:
    tools.fixup_referencers(redirectors)
ue.EditorAssetLibrary.save_directory("/Game", only_if_is_dirty=True, recursive=True)
left = [str(d.package_name) for d in registry.get_assets_by_path("/Game", recursive=True) if str(d.asset_class_path.asset_name) == "ObjectRedirector"]
ue.log(f"rename_tiles: {len(redirectors)} redirectors fixed up, {len(left)} left: {left[:10]}")
