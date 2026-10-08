# Runs at editor startup (PythonScriptPlugin): registers MyGame's MCP play-testing toolset.
import unreal

if unreal.is_editor():
    from mygame_tools import toolset

    toolset.registration.register()
