"""The strict recorder's deliberate breaks (Ass Plan.md, Step 0c item 2): changes the gate must catch, at least one per
mechanic, among them a dropped class default and a stun applied before the damage.

Each break edits a property's text (UMyGameEditorLibrary's Get/SetPropertyTextForTest, what the editor's copy and paste
use) in memory in the test editor: on the class default, and for the player's own abilities and the player also on every
live instance in the play world (an ability granted earlier keeps its own copy; an enemy's is made from the default when
the scenario spawns it). `undo()` puts every old text back. Nothing is saved. The `strict-breaks` suite
(Tests/strict_breaks.py) runs each break's scenario before, with and after it.
"""
import re

import unreal

_saved = []  # (object, property, old text) or (effect class, "@granted", old tags), newest last
_lib = unreal.MyGameEditorLibrary

GA_ENEMY1 = "/Game/Blueprints/Chars/EMelee1/GA_DamageEnemy1.GA_DamageEnemy1_C"
GA_DEBUFF = "/Game/Blueprints/Chars/Boss1/GA_BossSelfDebuff.GA_BossSelfDebuff_C"
GA_UPPER = "/Game/Abilities/GA_Upper.GA_Upper_C"
GA_PUNCHES = "/Game/Abilities/GA_BasicPunches.GA_BasicPunches_C"
GA_VOADORA = "/Game/Abilities/GA_Voadora.GA_Voadora_C"
GA_DASH = "/Game/Abilities/GA_Dash.GA_Dash_C"
BP_CHAR = "/Game/Blueprints/Chars/BP_Char.BP_Char_C"
GE_IMMUNE_PROJ = "/Game/Abilities/GE_ImmuneToProjectile.GE_ImmuneToProjectile_C"


def _targets(class_path, instances):
    cls = unreal.load_class(None, class_path)
    out = [unreal.get_default_object(cls)]
    if instances:
        for o in unreal.ObjectIterator(cls):
            try:
                # an instance already destroyed (its ability was cleared) and not yet collected has no object
                if unreal.SystemLibrary.is_valid(o) and "UEDPIE" in o.get_path_name() and not o.get_name().startswith("Default__"):
                    out.append(o)
            except Exception:
                continue
    return out


def _edit(objects, prop, change):
    for obj in objects:
        old = _lib.get_property_text_for_test(obj, prop)
        new = change(old)
        if new == old:
            raise RuntimeError(f"the break changes nothing in {obj.get_path_name()}.{prop}: {old[:200]}")
        _saved.append((obj, prop, old))
        if not _set_text(obj, prop, new):
            raise RuntimeError(f"couldn't set {obj.get_path_name()}.{prop} to {new[:200]}")


def _set_text(obj, prop, text):
    try:
        return _lib.set_property_text_for_test(obj, prop, text)
    except Exception as e:
        raise RuntimeError(f"setting {prop} on {obj!r} ({type(obj).__name__}): {e}") from None


def _edit_class(class_path, prop, change, instances=False):
    _edit(_targets(class_path, instances), prop, change)


def _holders(objects, prop):
    """Where a combat setting lives on each of objects: the object itself, or one of its subobjects (a component; the
    combat settings leave AMyCharacter for its combat component in Step 1)."""
    out = []
    for obj in objects:
        if _lib.get_property_text_for_test(obj, prop):
            out.append(obj)
            continue
        subs = [unreal.find_object(None, p) for p in _lib.list_subobjects(obj)]
        found = [s for s in subs if s and _lib.get_property_text_for_test(s, prop)]
        if len(found) != 1:
            raise RuntimeError(f"{obj.get_path_name()}: {len(found)} holders of {prop}")
        out += found
    return out


def _edit_combat_setting(class_path, prop, change, instances=False):
    _edit(_holders(_targets(class_path, instances), prop), prop, change)


def _items(text):
    """An exported array's items, "(a,b)" -> ["a", "b"] (split at the top level, outside quotes)."""
    if not text.startswith("(") or not text.endswith(")"):
        return []
    out, cur, depth, quoted = [], "", 0, False
    for ch in text[1:-1]:
        if ch == '"':
            quoted = not quoted
        elif not quoted and ch == "(":
            depth += 1
        elif not quoted and ch == ")":
            depth -= 1
        elif not quoted and ch == "," and depth == 0:
            out.append(cur)
            cur = ""
            continue
        cur += ch
    return out + ([cur] if cur else [])


def _join(items):
    return "(" + ",".join(items) + ")"


def _set_magnitude(effect, tag, value):
    """In the effect containers' text: the magnitude of tag on the container of effect (e.g. "GE_Launch_C")."""
    def change(text):
        items = _items(text)
        for i, item in enumerate(items):
            if f".{effect}" in item:
                items[i] = re.sub(r'(TagName="' + re.escape(tag) + r'"\),Magnitude=)[-0-9.]+', lambda m: m.group(1) + f"{value:.6f}", item)
        return _join(items)
    return change


def _without_tag(tag):
    def change(text):
        text = re.sub(r'\(TagName="' + re.escape(tag) + r'"\),?', "", text)
        return text.replace(",)", ")")
    return change


def _hitbox_radius(montage_path, radius):
    # the montage's notifies are its subobjects (its notify list is protected from scripts); a montage can also hold
    # leftover notify objects no track uses
    states = [unreal.find_object(None, p) for p in _lib.list_subobjects(unreal.load_asset(montage_path))]
    states = [s for s in states if s and "Hitbox" in s.get_class().get_name() and _lib.get_property_text_for_test(s, "Hitboxes")]
    if not states:
        raise RuntimeError(f"no hit shapes in {montage_path}")

    def change(text):
        # a shape's text leaves out what's at its default (SphereRadius 40)
        return _join([re.sub(r"SphereRadius=[0-9.]+", f"SphereRadius={radius:.6f}", i) if "SphereRadius=" in i
                      else f"(SphereRadius={radius:.6f}," + i[1:] for i in _items(text)])
    _edit(states, "Hitboxes", change)


def _granted_tags(effect_path, tags):
    cls = unreal.load_class(None, effect_path)
    was = _lib.set_effect_granted_tags_for_test(cls, tags)
    _saved.append((cls, "@granted", was))


# name: (the scenario of Tests/strict.py it runs, what it changes, the change)
BREAKS = {
    "dropped_class_default": ("enemy1_attack", "GA_DamageEnemy1 (EMelee1) HitPause back to the C++ default 0.5",
                              lambda: _edit_class(GA_ENEMY1, "HitPause", lambda t: "0.500000")),
    "stun_before_damage": ("enemy1_attack", "GA_DamageEnemy1 (EMelee1) applies GE_HitStun before GE_Damage",
                           lambda: _edit_class(GA_ENEMY1, "EffectsToApply",
                                               lambda t: _join(sorted(_items(t), key=lambda i: ".GE_HitStun_C" not in i)))),
    "nth_stun": ("nth_stun", "the player's MaxStuns 3 -> 4", lambda: _edit_combat_setting(BP_CHAR, "MaxStuns", lambda t: "4", True)),
    "stun_immune_target": ("stun_immune_boss", "GA_BossSelfDebuff no longer applies GE_StunImmune",
                           lambda: _edit_class(GA_DEBUFF, "EffectsToApply",
                                               lambda t: _join([i for i in _items(t) if ".GE_StunImmune_C" not in i]))),
    "kill": ("kill", "GA_BasicPunches' damage 15 -> 4 (the dummy, at 5 health, survives)",
             lambda: _edit_class(GA_PUNCHES, "EffectsToApply", _set_magnitude("GE_Damage_C", "data.damage", -4.0), True)),
    "kill_launch": ("kill_launch", "GA_Upper's launch: data.launch.z 800 -> 700",
                    lambda: _edit_class(GA_UPPER, "EffectsToApply", _set_magnitude("GE_Launch_C", "data.launch.z", 700.0), True)),
    "two_targets": ("two_targets", "PunchR_Montage's hit sphere radius 45 -> 40", lambda: _hitbox_radius("/Game/Anims/PunchR_Montage", 40.0)),
    "projectile_immune": ("projectile_immune", "GE_ImmuneToProjectile grants no tag",
                          lambda: _granted_tags(GE_IMMUNE_PROJ, unreal.GameplayTagContainer())),
    "attack_speed": ("attack_speed", "GA_BasicPunches' MontagesSpeed 1 -> 1.05",
                     lambda: _edit_class(GA_PUNCHES, "MontagesSpeed", lambda t: "1.050000", True)),
    "held_release": ("held_release", "GA_BasicPunches cancels without a landed hit",
                     lambda: _edit_class(GA_PUNCHES, "bNeedsHitToCancel", lambda t: "False", True)),
    "air_ground_learn": ("air_ground_learn", "GA_Voadora loses activate.flyingattack (it counts as a ground move)",
                         lambda: _edit_class(GA_VOADORA, "AbilityTags", _without_tag("activate.flyingattack"))),
    "graph_ability": ("dash", "GA_Dash's DashDistance 1100 -> 1000",
                      lambda: _edit_class(GA_DASH, "DashDistance", lambda t: "1000.000000", True)),
    "off_axis": ("off_axis", "PunchR_Montage's hit spheres 45 -> 20 (they miss a target off the axis)", lambda: _hitbox_radius("/Game/Anims/PunchR_Montage", 20.0)),
}


def apply(name):
    """Applies a break; returns its description and what it edited (object, property, new text)."""
    first = len(_saved)
    BREAKS[name][2]()
    edited = [f"{o.get_path_name()}.{p} = {_lib.get_property_text_for_test(o, p)[:160] if p != '@granted' else '(tags)'}" for o, p, _ in _saved[first:]]
    return BREAKS[name][1] + " [" + "; ".join(edited) + "]"


def undo():
    while _saved:
        obj, prop, old = _saved.pop()
        if prop == "@granted":
            _lib.set_effect_granted_tags_for_test(obj, old)
        elif not _set_text(obj, prop, old):
            raise RuntimeError(f"couldn't restore {obj.get_path_name()}.{prop}")
