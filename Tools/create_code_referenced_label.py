"""Creates /Game/Blueprints/PAL_CodeReferenced: a Primary Asset Label that always cooks the assets C++ refers to by
path only (a soft path set in a native constructor isn't a reference the cooker can see).

Keep it in sync with the TEXT("/Game/...") paths in Source (grep FSoftObjectPath).
Run with the editor closed:
    UnrealEditor-Cmd.exe MyGame.uproject -run=pythonscript -script="<abs path>/Tools/create_code_referenced_label.py" -unattended -nullrhi
"""
import unreal as ue

PATH, NAME = "/Game/Blueprints", "PAL_CodeReferenced"
ASSETS = [
    "/Game/Blueprints/Chars/BP_HitBox.BP_HitBox",  # UMyGameplayAbility::HitBoxClass
    "/Game/UI/BP_UMGActor.BP_UMGActor",            # APickup::WidgetActorClass
]

label = ue.load_asset(f"{PATH}/{NAME}")
if not label:
    factory = ue.DataAssetFactory()
    factory.set_editor_property("data_asset_class", ue.PrimaryAssetLabel)
    label = ue.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, ue.PrimaryAssetLabel, factory)
label.set_editor_property("explicit_assets", [ue.load_asset(a) for a in ASSETS])
label.set_editor_property("label_assets_in_my_directory", False)
label.set_editor_property("is_runtime_label", True)
rules = label.get_editor_property("rules")
rules.set_editor_property("cook_rule", ue.PrimaryAssetCookRule.ALWAYS_COOK)
label.set_editor_property("rules", rules)
ue.EditorAssetLibrary.save_loaded_asset(label)
ue.log(f"label {label.get_path_name()}: {[a.get_path_name() for a in label.get_editor_property('explicit_assets')]}")
