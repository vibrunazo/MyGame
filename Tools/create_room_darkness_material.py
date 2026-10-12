"""Creates /Game/FX/PostProcess/PP_RoomDarkness: a post-process material that darkens every pixel whose world position
is outside a rectangle (the current room), with a soft edge. ARoomCameraPawn drives its parameters: RoomMinX, RoomMinY,
RoomMaxX, RoomMaxY, Softness (fade width) and Brightness (what's left outside, 0 = black).

Run with the editor closed:
    UnrealEditor-Cmd.exe MyGame.uproject -run=pythonscript -script="<abs path>/Tools/create_room_darkness_material.py" -unattended -nullrhi
"""
import unreal as ue

PATH, NAME = "/Game/FX/PostProcess", "PP_RoomDarkness"
MEL = ue.MaterialEditingLibrary

mat = ue.load_asset(f"{PATH}/{NAME}")
if not mat:
    mat = ue.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, ue.Material, ue.MaterialFactoryNew())
MEL.delete_all_material_expressions(mat)
mat.set_editor_property("material_domain", ue.MaterialDomain.MD_POST_PROCESS)
mat.set_editor_property("blendable_location", ue.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)

scene = MEL.create_material_expression(mat, ue.MaterialExpressionSceneTexture, -700, 0)
scene.set_editor_property("scene_texture_id", ue.SceneTextureId.PPI_POST_PROCESS_INPUT0)
world = MEL.create_material_expression(mat, ue.MaterialExpressionWorldPosition, -700, 150)

params = {}
for i, (name, default) in enumerate([("RoomMinX", -700.0), ("RoomMinY", -1000.0), ("RoomMaxX", 700.0), ("RoomMaxY", 1000.0),
                                      ("Softness", 200.0), ("Brightness", 0.08)]):
    p = MEL.create_material_expression(mat, ue.MaterialExpressionScalarParameter, -700, 260 + i * 70)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", default)
    params[name] = p

custom = MEL.create_material_expression(mat, ue.MaterialExpressionCustom, -300, 100)
custom.set_editor_property("output_type", ue.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("description", "Darken outside the room")
inputs = []
for n in ["Color", "Pos", "MinX", "MinY", "MaxX", "MaxY", "Soft", "Bright"]:
    ci = ue.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
custom.set_editor_property("inputs", inputs)
custom.set_editor_property("code", "\n".join([
    "// distance outside the room's rectangle (0 inside), faded over Soft",
    "float2 lo = float2(MinX, MinY), hi = float2(MaxX, MaxY);",
    "float2 d = max(max(lo - Pos.xy, Pos.xy - hi), 0.0);",
    "float inside = saturate(1.0 - length(d) / max(Soft, 1.0));",
    "return Color * lerp(Bright, 1.0, inside);",
]))

MEL.connect_material_expressions(scene, "Color", custom, "Color")
MEL.connect_material_expressions(world, "", custom, "Pos")
for pin, name in [("MinX", "RoomMinX"), ("MinY", "RoomMinY"), ("MaxX", "RoomMaxX"), ("MaxY", "RoomMaxY"), ("Soft", "Softness"), ("Bright", "Brightness")]:
    MEL.connect_material_expressions(params[name], "", custom, pin)
MEL.connect_material_property(custom, "", ue.MaterialProperty.MP_EMISSIVE_COLOR)

MEL.recompile_material(mat)
ue.EditorAssetLibrary.save_loaded_asset(mat)
ue.log(f"created {mat.get_path_name()} with {MEL.get_num_material_expressions(mat)} expressions")
