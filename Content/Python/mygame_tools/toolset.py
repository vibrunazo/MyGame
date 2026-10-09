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


def _pawn_response(char):
    """The capsule's collision response to the Pawn channel (Block / Overlap / Ignore)."""
    capsule = char.get_editor_property("capsule_component")
    return str(capsule.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN)).split(".")[-1].rstrip(">").split(":")[0]


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
        state = {"actor": char.get_name(), "location": _vec(char.get_actor_location()),
                 "pawn_collision": _pawn_response(char)}
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
    def learn_item_abilities(item_asset_path: str) -> str:
        """Gives the PIE player the abilities of a learn item (e.g. "/Game/Blueprints/Items/DA_LearnTat"), as picking it up would.

        Args:
            item_asset_path: A LearnItemDataAsset.

        Returns:
            JSON list of the abilities learned (class and input slot).
        """
        char = _player()
        item = unreal.load_asset(item_asset_path)
        abilities = list(item.get_editor_property("abilities_to_learn"))
        char.learn_abilities(abilities)
        return json.dumps([{"ability": a.get_editor_property("ability_class").get_name(),
                            "input": str(a.get_editor_property("input")), "event": a.get_editor_property("event_name"),
                            "ground": a.get_editor_property("can_use_on_ground"), "air": a.get_editor_property("can_use_on_air")}
                           for a in abilities])

    @toolset_registry.tool_call
    @staticmethod
    def give_item(item_asset_path: str) -> str:
        """Gives the PIE player an item as picking it up would (e.g. "/Game/Blueprints/Items/DA_ItemFireHands").

        Args:
            item_asset_path: An ItemDataAsset.

        Returns:
            The player's state afterwards, as JSON.
        """
        char = _player()
        char.add_item_to_inventory(unreal.load_asset(item_asset_path))
        return MyGameTools.pie_player_state()

    @toolset_registry.tool_call
    @staticmethod
    def set_input_mods(super_mod: bool, ultra_mod: bool) -> str:
        """Holds or releases the Super (Shift/R1) and Ultra (Ctrl/R2) modifiers on the player controller.

        Args:
            super_mod: Hold the Super modifier.
            ultra_mod: Hold the Ultra modifier.

        Returns:
            The controller's modifier state as JSON.
        """
        controller = _player().get_controller()
        controller.set_super_mod(super_mod)
        controller.set_ultra_mod(ultra_mod)
        return json.dumps({"super": controller.get_super_mod(), "ultra": controller.get_ultra_mod()})

    @toolset_registry.tool_call
    @staticmethod
    def jump_player() -> str:
        """Makes the PIE player jump, as the jump button would.

        Returns:
            The player's state right after the jump, as JSON.
        """
        _player().jump()
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
            entry["pawn_collision"] = _pawn_response(char)
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
        # GameplayAttributeData(value) silently zeroes both fields; set them by name
        data = unreal.GameplayAttributeData(base_value=value, current_value=value)
        attributes.set_editor_property(attribute.lower(), data)
        return MyGameTools.pie_player_state()

    @toolset_registry.tool_call
    @staticmethod
    def spawn_fx(asset_path: str, x: float, y: float, z: float, scale: float = 1.0) -> str:
        """Spawns a Niagara or Cascade particle system at a PIE world location (e.g. to compare effects).

        Args:
            asset_path: The NiagaraSystem or ParticleSystem asset path.
            x: World X.
            y: World Y.
            z: World Z.
            scale: Uniform scale.

        Returns:
            The spawned component's path.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        asset = unreal.load_asset(asset_path)
        loc = unreal.Vector(x, y, z)
        size = unreal.Vector(scale, scale, scale)
        if isinstance(asset, unreal.NiagaraSystem):
            comp = unreal.NiagaraFunctionLibrary.spawn_system_at_location(world, asset, loc, unreal.Rotator(), size)
        elif isinstance(asset, unreal.ParticleSystem):
            comp = unreal.GameplayStatics.spawn_emitter_at_location(world, asset, loc, unreal.Rotator(), size)
        else:
            raise RuntimeError(f"not a particle system: {asset_path}")
        return comp.get_path_name() if comp else ""

    @toolset_registry.tool_call
    @staticmethod
    def floor_height(x: float, y: float, from_z: float = 1000.0) -> str:
        """Traces straight down at (x, y) in the PIE world against world geometry to find the floor.

        Args:
            x: World X.
            y: World Y.
            from_z: Height to trace down from; start just above the spot so roofs and ceilings aren't hit.

        Returns:
            JSON {"hit": bool, "z": floor height, "actor": what was hit}.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        start, end = unreal.Vector(x, y, from_z), unreal.Vector(x, y, -5000)
        # world geometry only (WorldStatic, WorldDynamic), so characters standing there don't count as floor
        hit = unreal.SystemLibrary.line_trace_single_for_objects(
            world, start, end, [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2],
            False, [], unreal.DrawDebugTrace.NONE, True)
        if not hit:
            return json.dumps({"hit": False})
        t = hit.to_tuple()
        # HitResult tuple: (blocking_hit, initial_overlap, time, distance, location, impact_point, normal, impact_normal, phys_mat, hit_actor, ...)
        return json.dumps({"hit": bool(t[0]), "z": round(t[4].z, 1), "actor": t[9].get_name() if t[9] else None})

    @toolset_registry.tool_call
    @staticmethod
    def health_bar_durations(actor_path: str) -> str:
        """Lists the duration bars (buff/debuff timers) on a character's health bar widget and the icon each one shows.

        Args:
            actor_path: Full path of a MyCharacter in the PIE world (see list_characters).

        Returns:
            JSON list of bars: widget class, parent panel, effect UI name, icon texture and color.
        """
        char = unreal.find_object(None, actor_path)
        if not char:
            raise RuntimeError(f"no actor at {actor_path}")
        widget = char.get_editor_property("health_bar_comp").get_user_widget_object()
        if not widget:
            return json.dumps({"error": "health bar widget not created"})
        bars = []
        for handle, bar in widget.get_editor_property("map_of_bars").items():
            if not bar:
                bars.append({"bar": None})  # effects without UI data get no bar
                continue
            entry = {"bar": bar.get_class().get_name(), "in_panel": bool(bar.get_parent())}
            with contextlib.suppress(Exception):
                ui = bar.get_editor_property("BuffUI")
                buff = ui.get_editor_property("buff_ui") if ui else None
                if buff:
                    icon = buff.get_editor_property("icon")
                    res = icon.get_editor_property("resource_object")
                    entry.update({"ui_class": ui.get_class().get_name(), "name": str(buff.get_editor_property("name")),
                                  "icon": res.get_path_name() if res else None,
                                  "color": str(buff.get_editor_property("color"))})
            bars.append(entry)
        return json.dumps(bars)

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
