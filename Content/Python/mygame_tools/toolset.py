"""MyGame play-testing hooks exposed through the Unreal MCP (ToolsetRegistry).

The shipped toolsets can inspect but not drive the game; these let agents press
the player's ability inputs and read back the result during PIE. Deliberately
narrow: no arbitrary code or console execution.
"""
import contextlib
import json

import unreal

import toolset_registry
from toolset_registry.registration import Registration

_ATTRIBUTES = ("Health", "MaxHealth", "Mana", "MaxMana", "Attack", "Defense", "Speed", "AttackSpeed", "RotSpeed")


def _pie_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


def _player():
    world = _pie_world()
    if not world:
        raise RuntimeError("PIE is not running")
    char = unreal.GameplayStatics.get_player_character(world, 0)
    if not char:
        raise RuntimeError("PIE has no player character yet")
    return char


def _vec(v: unreal.Vector):
    return [round(v.x, 1), round(v.y, 1), round(v.z, 1)]


@unreal.uclass()
class MyGameTools(unreal.ToolsetDefinition):
    """MyGame play-testing hooks: press the PIE player's ability inputs and read its state."""

    @toolset_registry.tool_call
    @staticmethod
    def pie_player_state() -> str:
        """Returns the PIE player's location, movement mode, attributes and owned gameplay tags.

        Returns:
            JSON snapshot of the player character.
        """
        char = _player()
        state = {"actor": char.get_name(), "location": _vec(char.get_actor_location())}
        movement = char.get_editor_property("character_movement")
        if movement:
            state["movement_mode"] = str(movement.get_editor_property("movement_mode"))
            state["velocity"] = _vec(movement.get_editor_property("velocity"))
        asc = char.get_editor_property("ability_system")
        attributes = char.get_editor_property("attribute_set_base") if asc else None
        if attributes:
            state["attributes"] = {
                name: attributes.get_editor_property(name.lower()).get_editor_property("current_value")
                for name in _ATTRIBUTES}
        if asc:
            with contextlib.suppress(Exception):
                state["tags"] = sorted(str(t.tag_name) for t in asc.get_owned_gameplay_tags().gameplay_tags)
        return json.dumps(state)

    @toolset_registry.tool_call
    @staticmethod
    def activate_ability_input(input_index: int) -> str:
        """Activates the player's ability bound to an input slot once, as a key press would.

        Args:
            input_index: EInput value: 0 Punch, 1 Kick, 2 Cast, 3 Jump (+10 Super, +20 Ultra).

        Returns:
            The player's owned tags right after activation, as JSON.
        """
        char = _player()
        char.activate_ability_by_input(input_index)
        return MyGameTools.pie_player_state()

    @toolset_registry.tool_call
    @staticmethod
    def activate_ability_event(event_name: str) -> str:
        """Activates the player's abilities registered under an event name (e.g. "dash").

        Args:
            event_name: The FAbilityStruct EventName to trigger.

        Returns:
            The player's state right after activation, as JSON.
        """
        char = _player()
        char.activate_ability_by_event(event_name)
        return MyGameTools.pie_player_state()

    @toolset_registry.tool_call
    @staticmethod
    def teleport_player(x: float, y: float, z: float) -> str:
        """Moves the PIE player to a world location (e.g. into another room).

        Args:
            x: World X.
            y: World Y.
            z: World Z.

        Returns:
            The player's state after the move, as JSON.
        """
        char = _player()
        char.set_actor_location(unreal.Vector(x, y, z), False, True)
        return MyGameTools.pie_player_state()

    @toolset_registry.tool_call
    @staticmethod
    def list_characters() -> str:
        """Lists every MyCharacter in the PIE world with location, team and health.

        Returns:
            JSON list of characters.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        cls = unreal.load_class(None, "/Script/MyGame.MyCharacter")
        chars = []
        for char in unreal.GameplayStatics.get_all_actors_of_class(world, cls):
            entry = {"actor": char.get_name(), "path": char.get_path_name(), "class": char.get_class().get_name(),
                     "location": _vec(char.get_actor_location()), "team": char.get_editor_property("team")}
            attributes = char.get_editor_property("attribute_set_base")
            if attributes:
                entry["health"] = attributes.get_editor_property("health").get_editor_property("current_value")
            chars.append(entry)
        return json.dumps(chars)


    @toolset_registry.tool_call
    @staticmethod
    def set_player_attribute(attribute: str, value: float) -> str:
        """Test cheat: sets base and current value of a player attribute (e.g. "Health", "MaxHealth").

        Writes the attribute data directly, bypassing GameplayEffects, so use it only to set up tests.

        Args:
            attribute: Attribute name on MyAttributeSet.
            value: New value.

        Returns:
            The player's state afterwards, as JSON.
        """
        char = _player()
        attributes = char.get_editor_property("attribute_set_base")
        attributes.set_editor_property(attribute.lower(), unreal.GameplayAttributeData(value))
        return MyGameTools.pie_player_state()

    @toolset_registry.tool_call
    @staticmethod
    def list_actors(class_path: str) -> str:
        """Lists PIE actors of a class (subclasses included) with path and location.

        Args:
            class_path: Class path, e.g. "/Script/MyGame.Pickup" or "/Game/Blueprints/Level/BP_Door.BP_Door_C".

        Returns:
            JSON list of actors.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        cls = unreal.load_class(None, class_path)
        if not cls:
            raise RuntimeError(f"class not found: {class_path}")
        return json.dumps([
            {"actor": a.get_name(), "path": a.get_path_name(), "class": a.get_class().get_name(),
             "location": _vec(a.get_actor_location()), "hidden": a.is_hidden_ed() if hasattr(a, "is_hidden_ed") else None}
            for a in unreal.GameplayStatics.get_all_actors_of_class(world, cls)])


registration = Registration([MyGameTools])
