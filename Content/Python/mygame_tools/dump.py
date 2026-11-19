"""Step 0c item 1: MyGame's combat and AI content, read in the editor, as JSON (the conversion's input).

Called through the MyGameTools hook dump_content(part); the `dumps` suite (Tests/dumps.py) writes each part under
Tests/baselines/dumps/ and compares. Every value is read by reflection (unreal.ToolsetLibrary), so the engine's own
fields come too; instanced subobjects (effect components, EQS generators and tests, blackboard key types, notify
objects) are expanded in place. Paths are package paths; floats are as the asset stores them.
"""
import json
import re

import unreal

TL = unreal.ToolsetLibrary
AL = unreal.AnimationLibrary
PACKS = ("/Game/AdvancedLocomotionV4", "/Game/ParagonMinions", "/Game/InfinityBladeEffects", "/Game/Spike")
# fields that describe a running instance, not the asset
RUNTIME = {"currentActivationInfo", "currentEventData", "currentActorInfo", "currentSpecHandle"}
CHARACTERS = ("/Game/Blueprints/Chars/BP_Char", "/Game/Blueprints/Chars/BP_EnemyBase", "/Game/Blueprints/Chars/EMelee1/BP_Enemy1",
              "/Game/Blueprints/Chars/EMelee2/BP_Enemy2", "/Game/Blueprints/Chars/EMelee3/BP_Enemy3",
              "/Game/Blueprints/Chars/EKicker/BP_EnemyKicker", "/Game/Blueprints/Chars/EWizard/BP_EnemyWizard",
              "/Game/Blueprints/Chars/Boss1/BP_Boss1")
ACTORS = ("/Game/Blueprints/AI/BP_AIController", "/Game/Blueprints/Chars/EKicker/BP_AIContKicker",
          "/Game/Blueprints/Chars/EWizard/BP_AIContWizard", "/Game/Blueprints/Chars/Boss1/BP_AIContBoss",
          "/Game/Blueprints/Chars/BP_HitBox", "/Game/Blueprints/Chars/BP_Projectile", "/Game/Blueprints/Chars/BP_Projectile1")


def _assets():
    """{asset class: [package paths]} of MyGame's own content (not the packs)."""
    out = {}
    for a in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path("/Game", recursive=True):
        p = str(a.package_name)
        if not p.startswith(PACKS):
            out.setdefault(str(a.asset_class_path.asset_name), []).append(p)
    return {k: sorted(v) for k, v in out.items()}


def _load(path):
    return unreal.load_asset(path)


def _parent(bp):
    """A Blueprint's parent class path, from the asset registry's ParentClass tag (not exposed to Python)."""
    data = unreal.AssetRegistryHelpers.get_asset_registry().get_asset_by_object_path(bp.get_path_name())
    value = str(data.get_tag_value("ParentClass") or "")
    return value.split("'")[1] if "'" in value else (value or None)


def flatten(obj, depth=0):
    """Every reflected property of obj, with instanced subobjects (whose path lies inside obj's) expanded."""
    if obj is None:
        return None
    listed = TL.list_struct_properties(obj.get_class())
    names = [n for n in json.loads(listed) if n not in RUNTIME] if listed else []
    values = json.loads(TL.get_object_properties(obj, names)) if names else {}
    if depth < 4:
        values = _expand(values, obj.get_path_name(), depth)
    return values


def _expand(v, owner_path, depth):
    if isinstance(v, dict):
        ref = v.get("refPath") if len(v) == 1 else None
        if isinstance(ref, str) and ref.startswith(owner_path) and ref != owner_path and ref[len(owner_path)] in ":.":
            sub = unreal.find_object(None, ref)
            if sub:
                return {"subobject": sub.get_class().get_name(), **flatten(sub, depth + 1)}
        # a reference reads as {"refPath": path} or as the bare path, depending on whether its target is loaded at the
        # time: the bare path either way, so a dump doesn't depend on what an earlier part or suite loaded
        if isinstance(ref, str):
            return ref
        return {k: _expand(x, owner_path, depth) for k, x in v.items()}
    if isinstance(v, list):
        return [_expand(x, owner_path, depth) for x in v]
    return v


def _class_entry(path):
    """A Blueprint class's parent and flattened class defaults."""
    bp = _load(path)
    cls = bp.generated_class()
    defaults = flatten(unreal.get_default_object(cls))
    ui = defaults.get("abilityUIdata")
    if isinstance(ui, dict) and isinstance(ui.get("icon"), dict):  # the brush's drawing settings are UI noise
        ui["icon"] = {"resourceObject": ui["icon"].get("resourceObject")}
    return {"asset_class": bp.get_class().get_name(), "parent": _parent(bp), "defaults": defaults}


def abilities():
    a = _assets()
    paths = [p for k in ("Blueprint", "GameplayAbilityBlueprint") for p in a.get(k, []) if p.split("/")[-1].startswith("GA_")]
    return {p: _class_entry(p) for p in sorted(paths)}


def effects():
    paths = [p for p in _assets().get("Blueprint", []) if p.split("/")[-1].startswith("GE_")]
    return {p: _class_entry(p) for p in sorted(paths)}


def actors():
    """The character Blueprints (the settings Step 1 moves, each enemy's abilities), the AI controllers (ranges, the
    boss's event responses), the hitbox and the projectiles."""
    return {p: _class_entry(p) for p in CHARACTERS + ACTORS}


def _curves(seq):
    out = {}
    for n in AL.get_animation_curve_names(seq, unreal.RawCurveTrackTypes.RCT_FLOAT):
        t, v = AL.get_float_keys(seq, n)
        out[str(n)] = [[a, b] for a, b in zip(t, v)]
    return out


def _notifies(anim):
    out = []
    for ev in AL.get_animation_notify_events(anim):
        state = ev.get_editor_property("notify_state_class")
        notify = state or ev.get_editor_property("notify")
        settings = flatten(notify) if notify else None
        # these two write the character's current slash or whoosh into the shared notify before each use
        # (the plugin's UAssSlashNotifyState and UAssCharacterSoundNotify), so the stored value is whatever played last
        if settings and settings.get("slashLocation") in ("Punch", "Kick") and "template" in settings:
            settings["template"] = "(set from the character's PunchSlash or KickSlash at NotifyBegin)"
        if settings and settings.get("wooshType") in ("PunchWeak", "PunchStrong") and "sound" in settings:
            settings["sound"] = "(set from the character's PunchWeakWoosh or PunchStrongWoosh at Notify)"
        out.append({"name": str(ev.get_editor_property("notify_name")),
                    "class": notify.get_class().get_path_name() if notify else None,
                    "start": AL.get_anim_notify_event_trigger_time(ev),
                    "end": AL.get_anim_notify_event_trigger_time(ev) + AL.get_anim_notify_event_duration(ev),
                    "track": int(re.search(r"TrackIndex=(-?\d+)", ev.export_text()).group(1)),
                    "settings": settings})
    return sorted(out, key=lambda n: (n["start"], n["track"], n["name"]))


def montages():
    """Each montage's timing: length, blends, sections (start, next), segments, notifies with their settings (a hit
    window's bones or sockets and shapes), and the curves of the montage and of its sequences."""
    out, seqs = {}, {}
    for p in _assets().get("AnimMontage", []):
        m = _load(p)
        tracks = json.loads(TL.get_object_properties(m, ["SlotAnimTracks", "BlendIn", "BlendOut", "BlendOutTriggerTime",
                                                         "bEnableAutoBlendOut", "RateScale", "SequenceLength"]))
        tracks = _expand(tracks, m.get_path_name(), 4)
        for t in tracks["SlotAnimTracks"]:
            for s in t["animTrack"]["animSegments"]:
                ref = s.get("animReference")
                if ref:
                    seqs.setdefault(ref.split(".")[0], None)
        out[p] = {"skeleton": m.get_editor_property("skeleton").get_path_name(),
                  "length": tracks.pop("SequenceLength"), **tracks,
                  "sections": json.loads(unreal.MyGameEditorLibrary.describe_montage_sections(m)),
                  "notifies": _notifies(m), "curves": _curves(m)}
    sequences = {}
    for s in sorted(seqs):
        a = _load(s)
        sequences[s] = {"length": a.get_play_length(), "frames": AL.get_num_frames(a), "root_motion": AL.is_root_motion_enabled(a),
                        "notifies": _notifies(a), "curves": _curves(a)}
    return {"montages": out, "sequences": sequences}


def _bt_node(node):
    entry = {"name": node.get_editor_property("node_name"), "class": node.get_class().get_path_name(), "settings": flatten(node)}
    services = list(node.get_editor_property("services") or []) if hasattr(node, "services") else []
    if services:
        entry["services"] = [_bt_node(s) for s in services]
    if isinstance(node, unreal.BTCompositeNode):
        entry["children"] = []
        for child in node.children:
            ops = []
            try:
                ops = [re.sub(r"^\(|\)$", "", op.export_text()) for op in child.get_editor_property("decorator_ops")]
            except Exception as e:  # not exposed: the decorators alone are then an AND
                ops = [f"(unreadable: {type(e).__name__})"]
            sub = child.child_composite or child.child_task
            entry["children"].append({"decorators": [_bt_node(d) for d in child.decorators], "decorator_ops": ops,
                                      "node": _bt_node(sub) if sub else None})
    return entry


def _bt(path):
    tree = _load(path)
    bb = tree.get_editor_property("blackboard_asset")
    return {"blackboard": bb.get_path_name() if bb else None,
            "root_decorators": [_bt_node(d) for d in tree.root_decorators],
            "root": _bt_node(tree.root_node) if tree.root_node else None}


def ai():
    """The trees (nested: each composite's children with their decorators, the decorators' logic and the services), the
    blackboard and the queries (options, generators and tests as subobjects)."""
    a = _assets()
    return {"trees": {p: _bt(p) for p in a.get("BehaviorTree", [])},
            "blackboards": {p: _with_subobjects(_load(p)) for p in a.get("BlackboardData", [])},
            "queries": {p: _with_subobjects(_load(p)) for p in a.get("EnvQuery", [])}}


def _with_subobjects(obj, depth=0):
    """obj's properties, and each of its subobjects (by name, recursively): a query's options with their generator and
    tests, a blackboard's key types."""
    out = {"properties": flatten(obj)}
    subs = {}
    for path in unreal.MyGameEditorLibrary.list_subobjects(obj):
        sub = unreal.find_object(None, path)
        if sub and depth < 3 and not sub.get_class().get_name().endswith(("Graph", "GraphNode")):  # editor-only graphs
            subs[path[len(obj.get_path_name()) + 1:]] = {"class": sub.get_class().get_path_name(), **_with_subobjects(sub, depth + 1)}
    if subs:
        out["subobjects"] = subs
    return out


def graphs():
    """Every graph of MyGame's own Blueprints as the editor's graph script (BlueprintTools.read_graph_dsl), with each
    Blueprint's parent. Graphs that hold no nodes beyond their entry are left out."""
    from editor_toolset.toolsets.blueprint import BlueprintTools as BPT
    a = _assets()
    out = {}
    for kind in ("Blueprint", "GameplayAbilityBlueprint", "WidgetBlueprint", "AnimBlueprint"):
        for p in a.get(kind, []):
            bp = _load(p)
            entry = {"kind": kind, "parent": _parent(bp), "graphs": {}}
            for g in BPT.list_graphs(bp):
                try:
                    text = BPT.read_graph_dsl(g)
                except Exception as e:  # an anim graph or a widget graph the script form doesn't cover
                    text = f"(unreadable: {type(e).__name__})"
                if text.strip():
                    entry["graphs"][g.get_name()] = text
            out[p] = entry
    return out


def references():
    """Who refers to each ability, template, effect and montage (the asset registry's referencers): every asset that
    holds an ability class or grants a template, every user of an effect, every holder of a montage."""
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    a = _assets()
    opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                                 include_searchable_names=False, include_soft_management_references=False,
                                                 include_hard_management_references=False)
    out = {}
    for p in a.get("Blueprint", []) + a.get("GameplayAbilityBlueprint", []) + a.get("AnimMontage", []) + a.get("AssAbilityTemplate", []):
        name = p.split("/")[-1]
        if name.startswith(("GA_", "GE_", "AT_")) or p in a.get("AnimMontage", []):
            out[p] = sorted(str(r) for r in (ar.get_referencers(p, opts) or []))
    return out


PARTS = {"abilities": abilities, "effects": effects, "actors": actors, "montages": montages, "ai": ai, "graphs": graphs,
         "references": references}


def dump(part):
    return PARTS[part]()
