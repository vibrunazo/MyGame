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


def _inject(action_name, value_type, value):
    """Holds (value = (x, y)) or releases (value = None) /Game/Input/IA_<action_name> through the controller's test hook."""
    action = unreal.load_asset(f"/Game/Input/IA_{action_name}")
    x, y = value or (0.0, 0.0)
    _player().get_controller().inject_test_input(action, unreal.Vector2D(x, y), value is not None)


def _set_attribute(char, attribute, value):
    """Sets a MyAttributeSet attribute's base value through the ability system (effects and damage see it)."""
    attr = unreal.GameplayAttribute()
    attr.import_text(f'(AttributeName="{attribute}",Attribute=/Script/MyGame.MyAttributeSet:{attribute},AttributeOwner=None)')
    char.set_attribute_base_for_test(attr, value)


def _set_senses(char, controller, enabled):
    """Switches an AI's sight (AI Perception on its controller) on or off."""
    perception = controller.get_ai_perception_component() if controller else None
    if perception:
        perception.set_sense_enabled(unreal.AISense_Sight, enabled)


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
        """Returns the PIE player's location, facing, movement and jump state, and attributes (owned tags: GASToolsets GetActiveTags).

        Returns:
            JSON snapshot of the player character.
        """
        char = _player()
        state = {"actor": char.get_name(), "location": _vec(char.get_actor_location()),
                 "yaw": round(char.get_actor_rotation().yaw, 1),
                 "control_yaw": round(char.get_controller().get_control_rotation().yaw, 1) if char.get_controller() else None,
                 "pawn_collision": _pawn_response(char)}
        movement = char.get_editor_property("character_movement")
        if movement:
            state["can_jump"] = char.can_jump()
            state["jump_count"] = char.get_editor_property("jump_current_count")
            state["movement_mode"] = str(movement.get_editor_property("movement_mode"))
            state["velocity"] = _vec(movement.get_editor_property("velocity"))
        asc = char.get_editor_property("ability_system")
        attributes = char.get_editor_property("attribute_set_base") if asc else None
        if attributes:
            state["attributes"] = {
                name: attributes.get_editor_property(name.lower()).get_editor_property("current_value")
                for name in _ATTRIBUTES}
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
    def dash_player(forward: float = 0.0, right: float = 0.0) -> str:
        """Makes the PIE player dash, as a double tap would (zero direction keeps the current facing).

        Args:
            forward: Direction along world +X.
            right: Direction along world +Y.

        Returns:
            The player's state right after, as JSON.
        """
        _player().dash(unreal.Vector2D(forward, right))
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
                            "input": str(a.get_editor_property("input"))}
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
    def press_input(button: str, pressed: bool) -> str:
        """Presses or releases a player button by injecting its Enhanced Input action, as a real key would.

        Args:
            button: Punch, Kick, Cast, Jump, Super or Ultra.
            pressed: True to press (held until released), False to release.

        Returns:
            The player's state afterwards, as JSON.
        """
        name = {"Super": "SuperMod", "Ultra": "UltraMod"}.get(button, button)
        _inject(name, unreal.InputActionValueType.BOOLEAN, (1.0, 0.0) if pressed else None)
        return MyGameTools.pie_player_state()

    @toolset_registry.tool_call
    @staticmethod
    def move_input(forward: float, right: float) -> str:
        """Holds the move stick (IA_Move and IA_Dash) at a direction until called again; 0, 0 lets go.

        Args:
            forward: -1..1 along world +X.
            right: -1..1 along world +Y.

        Returns:
            The player's state afterwards, as JSON.
        """
        held = (forward, right) if forward or right else None
        # a real key or stick feeds both actions; IA_Dash fires on a double tap
        _inject("Move", unreal.InputActionValueType.AXIS2D, held)
        _inject("Dash", unreal.InputActionValueType.AXIS2D, held)
        return MyGameTools.pie_player_state()

    @toolset_registry.tool_call
    @staticmethod
    def player_anim() -> str:
        """The PIE player's current montage and section, plus its active abilities.

        Returns:
            JSON {"montage", "section", "active"}.
        """
        char = _player()
        anim = char.get_editor_property("mesh").get_anim_instance()
        montage = anim.get_current_active_montage() if anim else None
        state = {"montage": montage.get_name() if montage else None,
                 "section": str(anim.montage_get_current_section(montage)) if montage else None}
        return json.dumps(state)

    @toolset_registry.tool_call
    @staticmethod
    def set_ai_enabled(actor_path: str, enabled: bool, show_player: bool = True) -> str:
        """Stops or restarts a character's AI logic (behavior tree), e.g. to use an enemy as a still test dummy.

        Args:
            actor_path: Full path of a PIE character.
            enabled: False stops its behavior tree and senses; True restarts them.
            show_player: When enabling, also hand it the player as its target right away (skip seeing).

        Returns:
            JSON {"actor", "ai_running"}.
        """
        char = unreal.find_object(None, actor_path)
        if not char:
            raise RuntimeError(f"no actor at {actor_path}")
        controller = char.get_controller()
        brain = controller.get_editor_property("brain_component") if controller else None
        if not brain:
            raise RuntimeError(f"{actor_path} has no AI brain")
        # senses too: seeing the player aggroes the whole room, which restarts frozen trees
        _set_senses(char, controller, enabled)
        if enabled:
            brain.restart_logic()
            if show_player:  # with senses off while frozen it may never have seen the player
                with contextlib.suppress(Exception):
                    char.on_pawn_seen(_player())
        else:
            brain.stop_logic("test dummy")
            controller.stop_movement()  # a move the tree already started keeps going otherwise
        return json.dumps({"actor": char.get_name(), "ai_running": brain.is_running()})

    @toolset_registry.tool_call
    @staticmethod
    def ai_target(actor_path: str, clear: bool = False, set_player: bool = False) -> str:
        """An AI character's blackboard target (TargetChar): who it is after. Optionally forgets it first.

        Args:
            actor_path: Full path of a PIE AI character.
            clear: Clear the target (and the character's TargetEnemy) instead of reading it.
            set_player: Make the PIE player its target (blackboard and TargetEnemy) without waking its AI.

        Returns:
            JSON {"target": actor name or null}.
        """
        char = unreal.find_object(None, actor_path)
        if not char:
            raise RuntimeError(f"no actor at {actor_path}")
        controller = char.get_controller()
        bb = controller.get_editor_property("blackboard") if controller else None
        if not bb:
            raise RuntimeError(f"{actor_path} has no blackboard")
        if clear:
            bb.clear_value("TargetChar")
            char.set_target_enemy(None)
        if set_player:
            bb.set_value_as_object("TargetChar", _player())
            char.set_target_enemy(_player())
        target = bb.get_value_as_object("TargetChar")
        return json.dumps({"target": target.get_name() if target else None})

    @toolset_registry.tool_call
    @staticmethod
    def character_state(actor_path: str) -> str:
        """Location, current montage, health and mana of any PIE character (owned tags: GASToolsets GetActiveTags).

        Args:
            actor_path: Full path of a PIE character (see list_characters).

        Returns:
            JSON snapshot.
        """
        char = unreal.find_object(None, actor_path)
        if not char:
            raise RuntimeError(f"no actor at {actor_path}")
        state = {"actor": char.get_name(), "location": _vec(char.get_actor_location()), "yaw": round(char.get_actor_rotation().yaw, 1)}
        anim = char.get_editor_property("mesh").get_anim_instance()
        montage = anim.get_current_active_montage() if anim else None
        state["montage"] = montage.get_name() if montage else None
        attributes = char.get_editor_property("attribute_set_base")
        if attributes:
            state["health"] = attributes.get_editor_property("health").get_editor_property("current_value")
            state["mana"] = attributes.get_editor_property("mana").get_editor_property("current_value")
            state["defense"] = attributes.get_editor_property("defense").get_editor_property("current_value")
        asc = char.get_editor_property("ability_system")
        return json.dumps(state)

    @toolset_registry.tool_call
    @staticmethod
    def set_character_attribute(actor_path: str, attribute: str, value: float) -> str:
        """Test cheat: sets any PIE character's attribute base value through the ability system.

        Args:
            actor_path: Full path of a PIE character.
            attribute: Attribute name on MyAttributeSet (e.g. "Health").
            value: New value.

        Returns:
            The character's state afterwards, as JSON.
        """
        char = unreal.find_object(None, actor_path)
        if not char:
            raise RuntimeError(f"no actor at {actor_path}")
        _set_attribute(char, attribute, value)
        return MyGameTools.character_state(actor_path)

    @toolset_registry.tool_call
    @staticmethod
    def refresh_health(actor_path: str) -> str:
        """Runs a character's health-changed handling (health bar, HUD, health-threshold events), e.g. after set_character_attribute.

        Args:
            actor_path: Full path of a PIE character.

        Returns:
            The character's state afterwards, as JSON.
        """
        char = unreal.find_object(None, actor_path)
        if not char:
            raise RuntimeError(f"no actor at {actor_path}")
        char.update_health_bar()
        return MyGameTools.character_state(actor_path)

    @toolset_registry.tool_call
    @staticmethod
    def set_player_facing(yaw: float) -> str:
        """Turns the PIE player and its control rotation to a world yaw (0 = +X), as stick input would.

        Args:
            yaw: World yaw in degrees.

        Returns:
            The player's state afterwards, as JSON.
        """
        char = _player()
        rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)
        char.set_actor_rotation(rot, False)
        char.get_controller().set_control_rotation(rot)
        return MyGameTools.pie_player_state()

    @toolset_registry.tool_call
    @staticmethod
    def effect_buff_ui(effect_class_path: str) -> str:
        """What a gameplay effect's buff bar shows: its UI data component's GetBuffUI() (name, color, icon).

        Args:
            effect_class_path: A GameplayEffect Blueprint class, e.g. "/Game/Abilities/GE_FireDot.GE_FireDot_C".

        Returns:
            JSON {"asset", "name", "description", "color", "icon"}.
        """
        cls = unreal.load_class(None, effect_class_path)
        cdo = unreal.get_default_object(cls)
        comp = next((c for c in cdo.get_editor_property("ge_components") if isinstance(c, unreal.MyGameplayEffectUIData)), None)
        if not comp:
            return json.dumps({"error": "no MyGameplayEffectUIData component"})
        ui = comp.get_buff_ui()
        icon = ui.get_editor_property("icon").get_editor_property("resource_object")
        asset = comp.get_editor_property("buff_ui_asset")
        return json.dumps({"asset": asset.get_path_name() if asset else None, "name": str(ui.get_editor_property("name")),
                           "description": str(ui.get_editor_property("description")), "color": str(ui.get_editor_property("color")),
                           "icon": icon.get_path_name() if icon else None})

    @toolset_registry.tool_call
    @staticmethod
    def activate_ability_class(actor_path: str, ability_class_path: str) -> str:
        """Tries to activate one of a character's granted abilities by class (as an AI or trigger would).

        Args:
            actor_path: Full path of a PIE character.
            ability_class_path: e.g. "/Game/Blueprints/Chars/Boss1/GA_BossSummon1.GA_BossSummon1_C".

        Returns:
            JSON {"activated": bool}.
        """
        char = unreal.find_object(None, actor_path)
        cls = unreal.load_class(None, ability_class_path)
        if not char or not cls:
            raise RuntimeError("actor or ability class not found")
        return json.dumps({"activated": char.get_editor_property("ability_system").try_activate_ability_by_class(cls, True)})

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
    def teleport_actor(actor_path: str, x: float, y: float, z: float, yaw: float = -999.0) -> str:
        """Moves any PIE actor (e.g. a frozen enemy used as a dummy) to a world location.

        Args:
            actor_path: Full path of a PIE actor.
            x: World X.
            y: World Y.
            z: World Z.
            yaw: World yaw to face (0 = +X); -999 keeps the current facing.

        Returns:
            The actor's new location as JSON.
        """
        actor = unreal.find_object(None, actor_path)
        if not actor:
            raise RuntimeError(f"no actor at {actor_path}")
        actor.set_actor_location(unreal.Vector(x, y, z), False, True)
        if yaw != -999.0:
            rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)
            actor.set_actor_rotation(rot, True)
            controller = actor.get_controller() if hasattr(actor, "get_controller") else None
            if controller:  # enemies turn with their controller (bUseControllerRotationYaw)
                controller.set_control_rotation(rot)
        return json.dumps(_vec(actor.get_actor_location()))

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
        """Test cheat: sets a player attribute's base value (e.g. "Health", "MaxHealth") through the ability system.

        Args:
            attribute: Attribute name on MyAttributeSet.
            value: New value.

        Returns:
            The player's state afterwards, as JSON.
        """
        _set_attribute(_player(), attribute, value)
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
                ui = bar.get_editor_property("BuffUI")  # the widget's variable: the effect's UI data component
                buff = ui.get_buff_ui() if ui else None
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

    @toolset_registry.tool_call
    @staticmethod
    def roll_loot(actor_path: str, rolls: int) -> str:
        """Rolls an actor's LootComponent (e.g. a room's RoomMaster) without spawning anything, filtered by what the player knows.

        Args:
            actor_path: Full path of a PIE actor that has a LootComponent.
            rolls: How many times to roll.

        Returns:
            JSON: the loot table (item, drop rate) and how often each item came up.
        """
        actor = unreal.find_object(None, actor_path)
        if not actor:
            raise RuntimeError(f"no actor at {actor_path}")
        loot = actor.get_component_by_class(unreal.LootComponent)
        if not loot:
            raise RuntimeError(f"{actor_path} has no LootComponent")
        counts = {}
        for _ in range(max(1, rolls)):
            item = loot.get_random_item()
            name = item.get_name() if item else "None"
            counts[name] = counts.get(name, 0) + 1
        table = [{"item": d.get_editor_property("item").get_name() if d.get_editor_property("item") else None,
                  "drop_rate": d.get_editor_property("drop_rate")} for d in loot.get_editor_property("loot_table")]
        return json.dumps({"table": table, "counts": counts})


registration = Registration([MyGameTools])
