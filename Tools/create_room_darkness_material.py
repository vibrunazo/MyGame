"""Creates /Game/FX/PostProcess/PP_RoomDarkness: a post-process material that darkens every pixel outside the current
room. ARoomCamera feeds it RoomMask, a small texture over the floor's tiles (lit texels = the current room's tiles, a
few per tile; bilinear filtering makes the soft edge), and where that texture lies in the world: OriginX, OriginY
(world position of the texture's corner) and ExtentX, ExtentY (its world size). Brightness is what's left outside the
room (0 = black).

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
mask = MEL.create_material_expression(mat, ue.MaterialExpressionTextureObjectParameter, -700, 260)
mask.set_editor_property("parameter_name", "RoomMask")
mask.set_editor_property("texture", ue.load_asset("/Engine/EngineResources/WhiteSquareTexture"))

params = {}
for i, (name, default) in enumerate([("OriginX", -2100.0), ("OriginY", -3000.0), ("ExtentX", 4200.0), ("ExtentY", 6000.0),
                                      ("Brightness", 0.08)]):
    p = MEL.create_material_expression(mat, ue.MaterialExpressionScalarParameter, -700, 420 + i * 70)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", default)
    params[name] = p

custom = MEL.create_material_expression(mat, ue.MaterialExpressionCustom, -300, 100)
custom.set_editor_property("output_type", ue.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("description", "Darken outside the room")
inputs = []
for n in ["Color", "Pos", "Mask", "OX", "OY", "EX", "EY", "Bright"]:
    ci = ue.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
custom.set_editor_property("inputs", inputs)
custom.set_editor_property("code", "\n".join([
    "// the room mask's texel under this pixel's world position (u along world X, v along world Y)",
    "float2 uv = (Pos.xy - float2(OX, OY)) / max(float2(EX, EY), 1.0);",
    "float lit = Texture2DSampleLevel(Mask, MaskSampler, uv, 0).r;",
    "return Color * lerp(Bright, 1.0, lit);",
]))

MEL.connect_material_expressions(scene, "Color", custom, "Color")
MEL.connect_material_expressions(world, "", custom, "Pos")
MEL.connect_material_expressions(mask, "", custom, "Mask")
for pin, name in [("OX", "OriginX"), ("OY", "OriginY"), ("EX", "ExtentX"), ("EY", "ExtentY"), ("Bright", "Brightness")]:
    MEL.connect_material_expressions(params[name], "", custom, pin)
MEL.connect_material_property(custom, "", ue.MaterialProperty.MP_EMISSIVE_COLOR)

MEL.recompile_material(mat)
ue.EditorAssetLibrary.save_loaded_asset(mat)
print(f"created {mat.get_path_name()} with {MEL.get_num_material_expressions(mat)} expressions")
