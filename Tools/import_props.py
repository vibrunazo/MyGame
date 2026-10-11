"""Import the graybox room props built by SourceArt/props/room_prop.py into /Game/Props.

Run with the editor closed:
    UnrealEditor-Cmd.exe MyGame.uproject -run=pythonscript -script="<abs path>/Tools/import_props.py" -unattended -nullrhi
"""
import unreal as ue

SRC = "D:/docs/Unreal Projects/MyGame/SourceArt/props/out/"
DEST = "/Game/Props"
PROPS = ["crate", "barrel", "rocks", "pillar"]

tasks = []
for kind in PROPS:
    t = ue.AssetImportTask()
    t.set_editor_property("filename", f"{SRC}SM_Prop_{kind}.fbx")
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", f"SM_Prop{kind.capitalize()}")
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    tasks.append(t)
ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    ue.log(f"imported {list(t.get_editor_property('imported_object_paths'))}")
ue.EditorAssetLibrary.save_directory(DEST)
