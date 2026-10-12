"""Follow-up to rename_tiles.py: resaves every package that still references a redirector the rename left, then
removes those redirectors. Only the rename's own redirectors (the old names of renamed assets); redirectors that were
already in the project are left alone.

Run with the editor closed:
    UnrealEditor-Cmd.exe MyGame.uproject -run=pythonscript -script="<abs path>/Tools/fixup_tile_redirectors.py" -unattended -nullrhi
"""
import unreal as ue

PREEXISTING = {"Room00-05", "Room01", "Room02", "Room03", "Room09-01"}
registry = ue.AssetRegistryHelpers.get_asset_registry()
ours = [str(d.package_name) for d in registry.get_assets_by_path("/Game/Maps/Rooms", recursive=False)
        if str(d.asset_class_path.asset_name) == "ObjectRedirector" and str(d.asset_name) not in PREEXISTING]
ours += ["/Game/Blueprints/BP_RoomCameraPawn", "/Game/Blueprints/Level/BP_LevelBuilder", "/Game/PCG/PCG_RoomDressing"]
ours = [p for p in ours if ue.EditorAssetLibrary.does_asset_exist(p)]

opts = ue.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
referencers = set()
for p in ours:
    for r in registry.get_referencers(p, opts) or []:
        r = str(r)
        if r not in ours:
            referencers.add(r)
ue.log(f"fixup: {len(ours)} redirectors, referencers: {sorted(referencers)}")

for r in sorted(referencers):
    if r.startswith("/Game/Maps/") and ue.EditorAssetLibrary.does_asset_exist(r):
        world = ue.EditorLoadingAndSavingUtils.load_map(r)
        ue.EditorLoadingAndSavingUtils.save_map(world, r)
    else:
        asset = ue.EditorAssetLibrary.load_asset(r)
        if asset:
            ue.EditorAssetLibrary.save_loaded_asset(asset, False)
ue.EditorLoadingAndSavingUtils.new_blank_map(False)

removed = []
for p in ours:
    left = [str(r) for r in registry.get_referencers(p, opts) or [] if str(r) not in ours]
    if not left and ue.EditorAssetLibrary.delete_asset(p):
        removed.append(p)
ue.log(f"fixup: resaved {len(referencers)}, removed {len(removed)}, kept {sorted(set(ours) - set(removed))}")
