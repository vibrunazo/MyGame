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

# set_game_speed: the time dilation tests want, re-applied whenever the game resets it to 1 (after a floor loads)
_speed = {"value": 1.0, "handle": None}


def _speed_tick(delta_seconds):
    world = _pie_world()
    if world and _speed["value"] != 1.0 and unreal.GameplayStatics.get_global_time_dilation(world) == 1.0:
        unreal.GameplayStatics.set_global_time_dilation(world, _speed["value"])


# run_timeline: scripted inputs fired on the game clock and per-frame recording, all inside the editor's tick, so
# scenarios behave the same at any game speed (no MCP round trips between an input and the next frame)
_tl = {"handle": None, "spec": None, "t0": 0.0, "pending": [], "rec": None, "phase": "idle", "result": None}


def _char(path):
    return unreal.find_object(None, path)


def _tl_tick(delta_seconds):
    world = _pie_world()
    if not world or _tl["phase"] in ("idle", "done"):
        return
    spec, rec = _tl["spec"], _tl["rec"]
    now = unreal.GameplayStatics.get_time_seconds(world) - _tl["t0"]
    player, dummy = _player(), _char(spec["dummy"]) if spec.get("dummy") else None
    if _tl["phase"] == "run":
        # at most one input per frame, so two inputs never land as one simultaneous press
        if _tl["pending"] and _tl["pending"][0][0] <= now:
            _, kind, hook, kwargs = (_tl["pending"].pop(0) + [None, None])[:4]
            if kind == "call":
                getattr(MyGameTools, hook)(**(kwargs or {}))
        anim = player.get_editor_property("mesh").get_anim_instance()
        montage = anim.get_current_active_montage() if anim else None
        if montage:
            key = f"{montage.get_name()}:{anim.montage_get_current_section(montage)}"
            if not rec["timeline"] or rec["timeline"][-1] != key:
                rec["timeline"].append(key)
        rec["abilities"].update(player.get_active_abilities_for_test())
        rec["player_tags"].update(t for t in player.get_tags_for_test() if t.startswith(("state.", "status.")) and t != "state.combat")
        rec["player_z"] = max(rec["player_z"], player.get_actor_location().z)
        if dummy:
            rec["dummy_tags"].update(t for t in dummy.get_tags_for_test() if t.startswith(("status.", "state.")))
            rec["dummy_z"] = max(rec["dummy_z"], dummy.get_actor_location().z)
        if spec.get("trace"):
            # one row per frame: game time, montage section, dummy health, player-dummy distance
            hp = json.loads(MyGameTools.character_state(spec["dummy"]))["health"] if dummy else 0.0
            gap = (dummy.get_actor_location() - player.get_actor_location()).length() if dummy else 0.0
            pos = anim.montage_get_position(montage) if montage else -1.0
            rec.setdefault("trace", []).append([round(now, 4), rec["timeline"][-1] if rec["timeline"] else "", round(hp, 1), round(gap, 1),
                                                round(player.get_editor_property("custom_time_dilation"), 3), round(pos, 3),
                                                round(player.get_actor_location().x, 1), round(player.get_actor_location().y, 1)])
        if now >= spec["duration"]:
            _tl["phase"], _tl["settle_from"] = "settle", now
    elif _tl["phase"] == "settle":
        # the outcome is read once the player's abilities have ended (the move is over, however long the frames
        # delayed it), and a combo window must close with the attack
        if not player.get_active_abilities_for_test() or now - _tl["settle_from"] >= spec.get("settle", 3.0):
            rec["end_player"] = json.loads(MyGameTools.pie_player_state())
            rec["end_dummy"] = json.loads(MyGameTools.character_state(spec["dummy"])) if dummy else None
            rec["lingering"] = sorted(t for t in player.get_tags_for_test() if t.startswith("combo."))
            _tl["phase"] = "done"
            _tl["result"] = {k: sorted(v) if isinstance(v, set) else v for k, v in rec.items()}


# video_frames: a screenshot request every PIE frame, with its game time
_vid = {"handle": None, "frames": [], "name": "", "t0": 0.0}


def _vid_tick(delta_seconds):
    world = _pie_world()
    if not world:
        return
    n = len(_vid["frames"])
    unreal.SystemLibrary.execute_console_command(world, f"HighResShot 1280x720 filename={_vid['name']}_{n:05d}")
    _vid["frames"].append(round(unreal.GameplayStatics.get_time_seconds(world) - _vid["t0"], 4))


# camera_record: samples taken every editor tick while recording (one per PIE frame)
_cam_rec = {"handle": None, "samples": [], "t0": 0.0}


def _cam_sample(delta_seconds):
    world = _pie_world()
    if not world:
        return
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    cams = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/MyGame.RoomCamera"))
    pcm = unreal.GameplayStatics.get_player_camera_manager(world, 0)
    if not pawn or not pcm:
        return
    loc, rot = pcm.get_camera_location(), pcm.get_camera_rotation()
    _cam_rec["samples"].append([round(unreal.GameplayStatics.get_time_seconds(world) - _cam_rec["t0"], 4),
                                *[round(c, 1) for c in (loc.x, loc.y, loc.z)], round(rot.pitch, 2), round(rot.yaw, 2),
                                round(pcm.get_fov_angle(), 2), *[round(c, 1) for c in _vec(pawn.get_actor_location())],
                                round(pawn.get_actor_rotation().yaw, 1), 1 if cams else 0])


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
    def level_state() -> str:
        """The generated level as the world sees it: streamed tiles (map, grid coordinate, loaded), walls, doors,
        room masters, enemies per room and the time dilation. Read from actors and streaming levels only, so it
        works whatever builds the level.

        Returns:
            JSON {"tiles", "rooms", "walls", "doors", "tile_masters", "enemies", "props", "time_dilation"}.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        builders = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/MyGame.FloorBuilder"))
        sx = builders[0].get_editor_property("tile_size_x") if builders else 2000.0
        sy = builders[0].get_editor_property("tile_size_y") if builders else 2000.0
        coord = lambda v: [round(v.x / sx), round(v.y / sy)]
        tiles = []
        for ls in unreal.ObjectIterator(unreal.LevelStreaming):
            if ls.get_world() != world or not ls.get_loaded_level():
                continue  # LevelPersistent also lists every room map as an unloaded sublevel
            # a level instance lives in a unique package (…_LevelInstance_N) but its world keeps the map's name
            tiles.append({"map": ls.get_loaded_level().get_outer().get_name(), "coord": coord(ls.get_editor_property("level_transform").translation),
                          "visible": ls.is_level_visible()})
        actors = lambda path: unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, path))
        walls = [{"at": [round(w.get_actor_location().x), round(w.get_actor_location().y)], "doored": w.get_editor_property("is_doored"),
                  "height": w.get_editor_property("height")} for w in actors("/Script/MyGame.Wall")]
        doors = [{"at": [round(d.get_actor_location().x), round(d.get_actor_location().y)],
                  "open": d.get_editor_property("box_collision").get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION}
                 for d in actors("/Script/MyGame.Door")]
        masters = [{"coord": coord(m.get_actor_location()), "chars": len(m.get_editor_property("chars_to_kill"))}
                   for m in actors("/Script/MyGame.TileMaster")]
        # props per room: mesh name -> instance count, from each room dressing's PCG-spawned instance components
        props = {}
        for d in actors("/Script/MyGame.TileDressing"):
            room = props.setdefault(",".join(map(str, coord(d.get_actor_location()))), {})
            for ism in d.get_components_by_class(unreal.InstancedStaticMeshComponent):
                if ism.static_mesh and ism.get_instance_count():
                    name = ism.static_mesh.get_name()
                    room[name] = room.get(name, 0) + ism.get_instance_count()
        enemies = {}
        for c in actors("/Script/MyGame.MyCharacter"):
            if c.get_editor_property("team") == 1:
                k = ",".join(map(str, coord(c.get_actor_location())))
                enemies[k] = enemies.get(k, 0) + 1
        # rooms: tiles joined with no wall between them, as the floor builder grouped them
        rooms = {}
        if builders:
            for t in tiles:
                r = builders[0].get_room_of_tile(unreal.IntPoint(t["coord"][0], t["coord"][1]))
                rooms.setdefault(r, []).append(t["coord"])
        return json.dumps({"tile_size": [sx, sy], "tiles": tiles, "rooms": sorted(sorted(v) for v in rooms.values()), "walls": walls, "doors": doors, "tile_masters": masters, "enemies": enemies, "props": props,
                           "time_dilation": unreal.GameplayStatics.get_global_time_dilation(world)})

    @toolset_registry.tool_call
    @staticmethod
    def run_state() -> str:
        """The run as the game instance and the PIE world see it: map, floor difficulty, seed, carried health and
        mana, items, and the player's learned abilities and current attributes.

        Returns:
            JSON {"map", "floor", "seed", "gi_health", "gi_mana", "items", "abilities", "player", "input_mode",
            "widgets"}; "player" is null when no player character is possessed (between levels, after death);
            "input_mode" is the controller's last SetInputMode ("Game Only" is the only one real keys and pads reach).
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        gi = unreal.GameplayStatics.get_game_instance(world)
        def prop(name):
            try:
                return gi.get_editor_property(name)
            except Exception:
                return None
        stream = prop("random_stream")
        state = {"map": unreal.GameplayStatics.get_current_level_name(world), "floor": prop("floor_number"),
                 "seed": stream.get_editor_property("initial_seed") if stream else None,
                 "gi_health": prop("health"), "gi_mana": prop("mana"),
                 "items": sorted(i.get_name() for i in prop("inventory") or [] if i), "abilities": None, "player": None}
        pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
        if pawn and hasattr(pawn, "learn_abilities"):  # a MyCharacter, not the spectator pawn shown after the level ends
            state["abilities"] = sorted(f'{a.get_editor_property("ability_class").get_name()}@{a.get_editor_property("input")}'
                                        for a in pawn.get_editor_property("abilities") if a.get_editor_property("ability_class"))
            attrs = pawn.get_editor_property("attribute_set_base")
            state["player"] = {n: attrs.get_editor_property(n.lower()).get_editor_property("current_value") for n in ("Health", "MaxHealth", "Mana")}
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        state["input_mode"] = pc.get_input_mode_for_test() if pc and hasattr(pc, "get_input_mode_for_test") else None
        state["widgets"] = sorted({w.get_class().get_name() for w in unreal.ObjectIterator(unreal.UserWidget)
                                   if w.get_world() == world and w.is_in_viewport()})
        return json.dumps(state)

    @toolset_registry.tool_call
    @staticmethod
    def hud_ability_icons(slots: str = "") -> str:
        """The icon texture each HUD ability button shows (WBP_HUD's buttons, set up from the abilities' UI data).

        Args:
            slots: Comma-separated EInput values (default: 0-3 and their Super +10 and Ultra +20 versions).

        Returns:
            JSON {slot: icon texture name or null}.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        hud = next((w for w in unreal.ObjectIterator(unreal.UserWidget)
                    if w.get_world() == world and w.get_class().get_name() == "WBP_HUD_C"), None)
        if not hud:
            raise RuntimeError("no HUD")
        out = {}
        for slot in [int(x) for x in slots.split(",") if x.strip()] or [0, 1, 2, 3, 10, 11, 12, 20, 21, 22]:
            button = hud.call_method("GetButtonReffromIndex", (slot,))
            icon = button.get_editor_property("Icon") if button else None
            res = icon.get_editor_property("brush").get_editor_property("resource_object") if icon else None
            out[str(slot)] = res.get_name() if res else None
        return json.dumps(out)

    @toolset_registry.tool_call
    @staticmethod
    def run_action(action: str) -> str:
        """Calls the game instance's run flow, as the menu buttons do.

        Args:
            action: start_new_run, next_floor or quit_to_main_menu.

        Returns:
            JSON {"called": action}.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        if action not in ("start_new_run", "next_floor", "quit_to_main_menu"):
            raise ValueError(f"unknown action {action}")
        getattr(unreal.GameplayStatics.get_game_instance(world), action)()
        return json.dumps({"called": action})

    @toolset_registry.tool_call
    @staticmethod
    def set_game_speed(speed: float) -> str:
        """Runs PIE `speed` times faster than real time (global time dilation), kept across floor loads, for tests.
        The game should behave the same at any speed.

        Args:
            speed: 1 for real time.

        Returns:
            JSON {"speed": speed}.
        """
        _speed["value"] = float(speed)
        if not _speed["handle"]:
            _speed["handle"] = unreal.register_slate_post_tick_callback(_speed_tick)
        world = _pie_world()
        if world and unreal.GameplayStatics.get_global_time_dilation(world) > 0.0:
            unreal.GameplayStatics.set_global_time_dilation(world, _speed["value"])
        return json.dumps({"speed": speed})

    @toolset_registry.tool_call
    @staticmethod
    def set_max_fps(fps: float) -> str:
        """Caps the frame rate (t.MaxFPS; 0 = uncapped), to test that gameplay doesn't depend on it.

        Args:
            fps: Frames per second, 0 for no cap.

        Returns:
            JSON {"max_fps": fps}.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        unreal.SystemLibrary.execute_console_command(world, f"t.MaxFPS {float(fps)}")
        return json.dumps({"max_fps": fps})

    @toolset_registry.tool_call
    @staticmethod
    def start_run(seed: int) -> str:
        """Starts a new run with this seed, as the main menu's New Game does (the floor reloads).

        Args:
            seed: The run's random seed.

        Returns:
            JSON {"seed": seed}.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        gi = unreal.GameplayStatics.get_game_instance(world)
        gi.set_editor_property("random_seed", int(seed))
        gi.start_new_run()
        return json.dumps({"seed": seed})

    @toolset_registry.tool_call
    @staticmethod
    def run_timeline(spec: str) -> str:
        """Plays scripted inputs on the game clock and records what happens, frame by frame, inside the editor.
        Poll timeline_result until it's done.

        Args:
            spec: JSON {"dummy": actor path or null, "duration": game seconds, "trace": true for a per-frame trace
                [t, montage section, dummy health, player-dummy distance,
                player time dilation, montage position], "settle": max game seconds to wait
                for the player's abilities to end afterwards, "inputs": [[t, "call", hook, {kwargs}], ...]}: at game time
                t, calls one of these MyGameTools hooks (press_input, move_input, dash_player, learn_item_abilities,
                give_item, set_character_attribute, set_player_attribute, refresh_health...).

        Returns:
            JSON {"started": true}.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        sp = json.loads(spec)
        player = _player()
        dummy = _char(sp["dummy"]) if sp.get("dummy") else None
        _tl.update(spec=sp, t0=unreal.GameplayStatics.get_time_seconds(world), phase="run", result=None,
                   pending=sorted([list(i) for i in sp["inputs"]], key=lambda i: i[0]),
                   rec={"timeline": [], "abilities": set(), "player_tags": set(), "dummy_tags": set(),
                        "player_z": player.get_actor_location().z,
                        "dummy_z": dummy.get_actor_location().z if dummy else 0.0})
        if not _tl["handle"]:
            _tl["handle"] = unreal.register_slate_post_tick_callback(_tl_tick)
        return json.dumps({"started": True})

    @toolset_registry.tool_call
    @staticmethod
    def timeline_result() -> str:
        """The recording of the last run_timeline.

        Returns:
            JSON {"done": bool, "result": {timeline, abilities, player_tags, dummy_tags, player_z, dummy_z,
            end_player, end_dummy, lingering} once done}.
        """
        return json.dumps({"done": _tl["phase"] == "done", "phase": _tl["phase"], "result": _tl["result"]})

    @toolset_registry.tool_call
    @staticmethod
    def game_time() -> str:
        """The PIE world's game time (seconds, slowed by time dilation), for scripts that wait in game time.

        Returns:
            JSON {"t": seconds}.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        return json.dumps({"t": unreal.GameplayStatics.get_time_seconds(world)})

    @toolset_registry.tool_call
    @staticmethod
    def video_frames(action: str, prefix: str = "vid", slowdown: float = 0.25) -> str:
        """Records the game camera as numbered screenshots, one per PIE frame (Saved/Screenshots/<platform>/<prefix>_NNNNN.png),
        with the game time of each, for turning into a video. Needs a rendering editor (-RenderOffscreen). Slows the
        game down while recording so every frame gets captured; scale scripted waits by 1/slowdown.

        Args:
            action: "start" or "stop".
            prefix: File name prefix.
            slowdown: Global time dilation while recording.

        Returns:
            "start": JSON {"recording": name}. "stop": JSON {"name", "times"} (game seconds per frame index).
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        if action == "start":
            if _vid["handle"]:
                unreal.unregister_slate_post_tick_callback(_vid["handle"])
            unreal.GameplayStatics.set_global_time_dilation(world, slowdown)
            _vid.update(frames=[], name=prefix, t0=unreal.GameplayStatics.get_time_seconds(world))
            _vid["handle"] = unreal.register_slate_post_tick_callback(_vid_tick)
            return json.dumps({"recording": prefix})
        if _vid["handle"]:
            unreal.unregister_slate_post_tick_callback(_vid["handle"])
            _vid["handle"] = None
        unreal.GameplayStatics.set_global_time_dilation(world, 1.0)
        return json.dumps({"name": _vid["name"], "times": _vid["frames"]})

    @toolset_registry.tool_call
    @staticmethod
    def camera_record(action: str) -> str:
        """Records what the game camera does, one sample per frame (the player camera manager's view, so whatever
        drives the camera), with the player's position and facing.

        Args:
            action: "start" (clears earlier samples) or "stop".

        Returns:
            "start": JSON {"recording": true}. "stop": JSON {"columns", "samples"}; each sample is
            [t, cam x, y, z, pitch, yaw, fov, player x, y, z, player yaw, room camera present].
        """
        if action == "start":
            if _cam_rec["handle"]:
                unreal.unregister_slate_post_tick_callback(_cam_rec["handle"])
            world = _pie_world()
            _cam_rec.update(samples=[], t0=unreal.GameplayStatics.get_time_seconds(world) if world else 0.0)
            _cam_rec["handle"] = unreal.register_slate_post_tick_callback(_cam_sample)
            return json.dumps({"recording": True})
        if _cam_rec["handle"]:
            unreal.unregister_slate_post_tick_callback(_cam_rec["handle"])
            _cam_rec["handle"] = None
        return json.dumps({"columns": ["t", "cx", "cy", "cz", "pitch", "yaw", "fov", "px", "py", "pz", "pyaw", "roomcam"],
                           "samples": _cam_rec["samples"]})

    @toolset_registry.tool_call
    @staticmethod
    def game_screenshot(name: str) -> str:
        """Saves what the PIE game camera shows (HighResShot) to Saved/Screenshots; needs a rendering editor
        (-RenderOffscreen). The file appears a frame or two later.

        Args:
            name: File name without extension.

        Returns:
            JSON {"requested": name}.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        unreal.SystemLibrary.execute_console_command(world, f"HighResShot 1280x720 filename={name}")
        return json.dumps({"requested": name})

    @toolset_registry.tool_call
    @staticmethod
    def regenerate_dressings() -> str:
        """Runs every room dressing's PCG graph again (after editing PCG_RoomDressing during PIE).

        Returns:
            JSON {"dressings": count}.
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        dressings = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/MyGame.TileDressing"))
        for d in dressings:
            d.get_component_by_class(unreal.PCGComponent).generate_local(True)
        return json.dumps({"dressings": len(dressings)})

    @toolset_registry.tool_call
    @staticmethod
    def room_bounds() -> str:
        """The geometry of each streamed room, relative to its grid cell's center: the box around everything that blocks
        (floors, platforms, stairs, props), and the box around the walkable tops (surfaces facing up).

        Returns:
            JSON [{"map", "cell", "solid": [min xyz, max xyz], "floor_z": [lowest, highest top], "actors"}].
        """
        world = _pie_world()
        if not world:
            raise RuntimeError("PIE is not running")
        builders = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/MyGame.FloorBuilder"))
        sx = builders[0].get_editor_property("tile_size_x") if builders else 2000.0
        sy = builders[0].get_editor_property("tile_size_y") if builders else 2000.0
        levels = {}
        for ls in unreal.ObjectIterator(unreal.LevelStreaming):
            if ls.get_world() == world and ls.get_loaded_level():
                t = ls.get_editor_property("level_transform").translation
                levels[ls.get_loaded_level().get_path_name()] = (ls.get_loaded_level().get_outer().get_name(), t)
        out = {}
        for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
            lvl = a.get_level().get_path_name() if a.get_level() else None
            if lvl not in levels:
                continue
            comp = a.static_mesh_component
            if not comp or comp.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION:
                continue
            name, t = levels[lvl]
            origin, extent = a.get_actor_bounds(True)
            lo = [origin.x - extent.x - t.x, origin.y - extent.y - t.y, origin.z - extent.z - t.z]
            hi = [origin.x + extent.x - t.x, origin.y + extent.y - t.y, origin.z + extent.z - t.z]
            r = out.setdefault(lvl, {"map": name, "cell": [round(t.x / sx), round(t.y / sy)], "lo": lo, "hi": hi, "tops": [], "actors": 0})
            r["lo"] = [min(x, y) for x, y in zip(r["lo"], lo)]
            r["hi"] = [max(x, y) for x, y in zip(r["hi"], hi)]
            r["tops"].append(hi[2])
            r["actors"] += 1
        return json.dumps([{"map": r["map"], "cell": r["cell"], "solid": [[round(v) for v in r["lo"]], [round(v) for v in r["hi"]]],
                            "floor_z": [round(min(r["tops"])), round(max(r["tops"]))], "actors": r["actors"]} for r in out.values()])

    @toolset_registry.tool_call
    @staticmethod
    def kill_character(actor_path: str) -> str:
        """Kills a PIE character through its normal death (OnDie: room masters, loot and doors react).

        Args:
            actor_path: Full path of a PIE character.

        Returns:
            JSON {"actor", "alive"}.
        """
        char = unreal.find_object(None, actor_path)
        if not char:
            raise RuntimeError(f"no actor at {actor_path}")
        char.on_die()
        return json.dumps({"actor": char.get_name(), "alive": char.is_alive()})

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
