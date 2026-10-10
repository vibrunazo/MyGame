"""Creates the Enhanced Input assets (/Game/Input) that replace the legacy action/axis mappings.

Run once with the editor's Python commandlet:
    UnrealEditor-Cmd.exe MyGame.uproject -run=pythonscript -script=Tools/create_input_assets.py
Keys mirror the old DefaultInput.ini mappings. IA_Move is 2D: X = forward (world +X), Y = right (world +Y).
"""
import unreal

PATH = "/Game/Input"
tools = unreal.AssetToolsHelpers.get_asset_tools()
log = []


def asset(name, cls, factory):
    full = f"{PATH}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        return unreal.load_asset(full)
    return tools.create_asset(name, PATH, cls, factory)


def action(name, value_type, when_paused=False):
    a = asset(name, unreal.InputAction, unreal.InputAction_Factory())
    a.set_editor_property("value_type", value_type)
    a.set_editor_property("trigger_when_paused", when_paused)
    unreal.EditorAssetLibrary.save_loaded_asset(a)
    return a


BOOL, AXIS2D = unreal.InputActionValueType.BOOLEAN, unreal.InputActionValueType.AXIS2D
move = action("IA_Move", AXIS2D)
buttons = {n: action(f"IA_{n}", BOOL) for n in ("Punch", "Kick", "Cast", "Jump", "SuperMod", "UltraMod", "ShowFPS")}
buttons["Pause"] = action("IA_Pause", BOOL, when_paused=True)

imc = asset("IMC_Default", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
imc.unmap_all()


def key(name):
    k = unreal.Key()
    k.set_editor_property("key_name", name)
    return k


def modifier(cls, **props):
    m = unreal.new_object(cls, outer=imc)
    for k, v in props.items():
        m.set_editor_property(k, v)
    return m


def add(act, key_name, mods=()):
    imc.map_key(act, key(key_name))
    data = imc.get_editor_property("default_key_mappings")
    mappings = list(data.get_editor_property("mappings"))
    mappings[-1].set_editor_property("modifiers", list(mods))
    data.set_editor_property("mappings", mappings)
    imc.set_editor_property("default_key_mappings", data)


swap = lambda: modifier(unreal.InputModifierSwizzleAxis, order=unreal.InputAxisSwizzle.YXZ)
neg = lambda: modifier(unreal.InputModifierNegate)
# X = forward, Y = right (the old MoveForward / MoveRight axes)
add(move, "W")
add(move, "S", [neg()])
add(move, "D", [swap()])
add(move, "A", [swap(), neg()])
# stick: X = right, Y = up; swap so up drives forward. Axial 0.25 dead zone like the old axis config
add(move, "Gamepad_Left2D", [modifier(unreal.InputModifierDeadZone, type=unreal.DeadZoneType.AXIAL, lower_threshold=0.25), swap()])

for name, keys in {
    "Punch": ["Left", "NumPadZero", "Gamepad_FaceButton_Left"],
    "Kick": ["Up", "NumPadThree", "Gamepad_FaceButton_Top"],
    "Cast": ["Right", "NumPadOne", "Gamepad_FaceButton_Right"],
    "Jump": ["SpaceBar", "Gamepad_FaceButton_Bottom"],
    "SuperMod": ["LeftShift", "RightShift", "Gamepad_RightShoulder"],
    "UltraMod": ["LeftControl", "RightControl", "Gamepad_RightTrigger"],
    "Pause": ["Escape", "F10", "P", "Pause", "Gamepad_Special_Left", "Gamepad_Special_Right"],
    "ShowFPS": ["F3"],
}.items():
    for k in keys:
        add(buttons[name], k)

unreal.EditorAssetLibrary.save_loaded_asset(imc)
for m in imc.get_editor_property("default_key_mappings").get_editor_property("mappings"):
    log.append(f"{m.get_editor_property('action').get_name()} {m.get_editor_property('key').get_editor_property('key_name')} "
               f"{[x.get_class().get_name() for x in m.get_editor_property('modifiers')]}")
unreal.log_warning("INPUTASSETS\n" + "\n".join(log))
