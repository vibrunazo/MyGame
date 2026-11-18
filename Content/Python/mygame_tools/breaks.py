"""The strict recorder's deliberate breaks (Ass Plan.md, Step 0c item 2): changes the gate must catch, at least one per
mechanic, among them a dropped class default and a stun applied before the damage.

Each break edits a property's text (UMyGameEditorLibrary's Get/SetPropertyTextForTest, what the editor's copy and paste
use) in memory in the test editor: on the class default, and for the player's own abilities and the player also on every
live instance in the play world (an ability granted earlier keeps its own copy; an enemy's is made from the default when
the scenario spawns it). Since Ass Plan.md Step 3 the plugin's abilities read their template's definition: a break of
one edits the template's definition (one object, which every grant reads), whose runtime object is then built again.
`undo()` puts every old value back. Nothing is saved. The `strict-breaks` suite (Tests/strict_breaks.py) runs each
break's scenario before, with and after it.
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


def _edit_template(class_path, change):
    """The definition of the template an ability class names: change(definition) edits a copy, which the template
    takes (its runtime object is built again on next use)."""
    template = unreal.get_default_object(unreal.load_class(None, class_path)).get_editor_property("template")
    # a struct read from an object can be the object's own memory: the change goes into a detached copy, so setting it
    # is a change the template hears (and builds its runtime object again)
    old = template.get_editor_property("definition").export_text()
    new = unreal.AssAbilityDefinition()
    new.import_text(old)
    change(new)
    if new.export_text() == old:
        raise RuntimeError(f"the break changes nothing in {template.get_path_name()}")
    _saved.append((template, "@definition", old))
    template.set_editor_property("definition", new)


def _set_in(struct, path, value):
    """struct.a.b.c = value, through copies (Python structs are values): path "a.b.c"."""
    head, _, rest = path.partition(".")
    if not rest:
        struct.set_editor_property(head, value)
        return
    inner = struct.get_editor_property(head)
    _set_in(inner, rest, value)
    struct.set_editor_property(head, inner)


def _payload(name_path, value):
    """The first payload's field (e.g. "damage.magnitude")."""
    def change(definition):
        payloads = definition.get_editor_property("payloads")
        first = payloads[0]  # a copy: written back below
        _set_in(first, name_path, value)
        payloads[0] = first
        definition.set_editor_property("payloads", payloads)
    return change


def _step_speeds(value):
    def change(definition):
        steps = definition.get_editor_property("steps")
        for index in range(len(steps)):
            step = steps[index]  # a copy: written back
            step.set_editor_property("speed", value)
            steps[index] = step
        definition.set_editor_property("steps", steps)
    return change


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
    "dropped_class_default": ("enemy1_attack", "GA_DamageEnemy1 (EMelee1)'s template: HitPause back to the C++ default 0.5",
                              lambda: _edit_template(GA_ENEMY1, lambda d: _set_in(d, "feel.hit_pause", 0.5))),
    # Step 3: a payload's parts apply in the plugin's fixed order, so "the stun before the damage" can't be authored;
    # the stun's own time takes its place
    "stun_time": ("enemy1_attack", "GA_DamageEnemy1 (EMelee1)'s template: the stun 0.35 s -> 0.6 s",
                  lambda: _edit_template(GA_ENEMY1, _payload("stun.time", 0.6))),
    "nth_stun": ("nth_stun", "the player's MaxStuns 3 -> 4", lambda: _edit_combat_setting(BP_CHAR, "MaxStuns", lambda t: "4", True)),
    "stun_immune_target": ("stun_immune_boss", "GA_BossSelfDebuff no longer applies GE_StunImmune",
                           lambda: _edit_class(GA_DEBUFF, "EffectsToApply",
                                               lambda t: _join([i for i in _items(t) if ".GE_StunImmune_C" not in i]))),
    "kill": ("kill", "GA_BasicPunches' template: damage 15 -> 4 (the dummy, at 5 health, survives)",
             lambda: _edit_template(GA_PUNCHES, _payload("damage.magnitude", 4.0))),
    "kill_launch": ("kill_launch", "GA_Upper's template: the launch's Z 800 -> 700",
                    lambda: _edit_template(GA_UPPER, _payload("launch.velocity.z", 700.0))),
    "two_targets": ("two_targets", "PunchR_Montage's hit sphere radius 45 -> 40", lambda: _hitbox_radius("/Game/Anims/PunchR_Montage", 40.0)),
    "projectile_immune": ("projectile_immune", "GE_ImmuneToProjectile grants no tag",
                          lambda: _granted_tags(GE_IMMUNE_PROJ, unreal.GameplayTagContainer())),
    "attack_speed": ("attack_speed", "GA_BasicPunches' template: every step's speed 1 -> 1.05",
                     lambda: _edit_template(GA_PUNCHES, _step_speeds(1.05))),
    "held_release": ("held_release", "GA_BasicPunches' template cancels without a landed hit",
                     lambda: _edit_template(GA_PUNCHES, lambda d: _set_in(d, "cancel.needs_hit_to_cancel", False))),
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
    edited = [f"{o.get_path_name()}.{p} = {_lib.get_property_text_for_test(o, p)[:160] if not p.startswith('@') else '(' + p[1:] + ')'}" for o, p, _ in _saved[first:]]
    return BREAKS[name][1] + " [" + "; ".join(edited) + "]"


def undo():
    while _saved:
        obj, prop, old = _saved.pop()
        if prop == "@granted":
            _lib.set_effect_granted_tags_for_test(obj, old)
        elif prop == "@definition":
            definition = unreal.AssAbilityDefinition()
            definition.import_text(old)
            obj.set_editor_property("definition", definition)
        elif not _set_text(obj, prop, old):
            raise RuntimeError(f"couldn't restore {obj.get_path_name()}.{prop}")
