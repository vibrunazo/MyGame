"""Step 0c item 1: MyGame's combat and AI content as read in the editor, kept as JSON baselines (Ass Plan.md, Step 0c).

The `dumps` suite of `unreal-agent test` (`suite(ctx)`). Each part comes from the MyGameTools hook dump_content
(Content/Python/mygame_tools/dump.py) and must equal its file under Tests/baselines/dumps/:

  abilities   every GA_*: its parent and flattened class defaults (the engine's fields too: tags, cost, cooldown,
              triggers, instancing; MyGame's: montages, payload effects in order, HitPause, cancels)
  effects     the 27 GE_*: duration, stacking, application rules, modifiers, executions and components
  actors      the character Blueprints (MaxStuns, StunImmuneCooldown, the hitbox class, each enemy's abilities), the
              AI controllers (ranges, the boss's event responses), the hitbox and the projectiles
  montages    each montage's length, sections, segments, notifies (Blueprint ones too) with their settings, curves
  ai          the trees (with the decorators' logic), the blackboard and the queries
  graphs      every graph of MyGame's own Blueprints, as the editor's graph script
  references  who refers to each ability, effect and montage
  callers     derived here: which Blueprint graphs name each UFUNCTION of what Step 1 moves
  tags        derived here: every gameplay tag, where it's declared, the assets that refer to it, and the C++ and
              config lines that name it (the tag table of 3.11; who takes each tag is filled in by the plan's steps)

A part that differs is a problem: the content or how it reads changed. When the change is meant, record it again
with `unreal-agent test --only dumps --accept dumps`, in a commit of its own.
"""
import json
import re
from pathlib import Path

import mygame
from mygame import call_tool, tool

ROOT = Path(__file__).resolve().parent.parent
BASE = ROOT / "Tests" / "baselines" / "dumps"
PARTS = ("abilities", "effects", "actors", "montages", "ai", "graphs", "references")
TAGS = "GameplayTagsToolset.GameplayTagsToolset"
# what Step 1 moved out of the character, and what the ability and the hitbox call on it (the last two in the plugin's
# AssCombat since Step 2)
STEP1_HEADERS = ("Source/MyGame/Player/MyCharacter.h", "Source/MyGame/Abilities/IGetHit.h",
                 "<plugin>/Source/AssCombat/Public/AssAbility.h", "<plugin>/Source/AssCombat/Public/AssHitBox.h")


def plugin_root():
    """The AbilityStrategySystem plugin this project mounts (its pin's folder, from the .uproject)."""
    project = json.loads((ROOT / "MyGame.uproject").read_text(encoding="utf-8"))
    for folder in project.get("AdditionalPluginDirectories", []):
        candidate = (ROOT / folder / "AbilityStrategySystem").resolve()
        if candidate.is_dir():
            return candidate
    raise FileNotFoundError("the .uproject mounts no AbilityStrategySystem")


def header_path(header):
    return plugin_root() / header[len("<plugin>/"):] if header.startswith("<plugin>/") else ROOT / header
# who keeps a tag family, where Plan.md 3.11 already says
FAMILY_OWNER = {"activate": "MyGame (3.11)", "buff": "MyGame (3.11)", "status.health": "MyGame (3.11)",
                "GameplayCue": "the plugin's cues sit under GameplayCue.Ass. (3.11)"}


def ufunctions(header):
    text = header_path(header).read_text(encoding="utf-8", errors="replace")
    return sorted(set(re.findall(r"UFUNCTION\([^)]*\)\s*(?:virtual\s+|static\s+|FORCEINLINE\s+)*[\w:<>*&,\s]+?\b(\w+)\s*\(", text)))


def callers(graphs):
    """For each UFUNCTION of the headers Step 1 moves: the Blueprint graphs that name it (a call, or an event it
    implements)."""
    out = {}
    for header in STEP1_HEADERS:
        for name in ufunctions(header):
            pattern = re.compile(r"\|" + re.escape(name) + r"[\s)]")
            hits = sorted(f"{bp} :: {g}" for bp, entry in graphs.items() for g, text in entry["graphs"].items() if pattern.search(text))
            out[f"{Path(header).stem}.{name}"] = hits
    return out


def native_names():
    """{tag: the C++ names of its native declarations} (UE_DEFINE_GAMEPLAY_TAG and its variants)."""
    out = {}
    for f in (ROOT / "Source").rglob("*.cpp"):
        text = f.read_text(encoding="utf-8", errors="replace")
        for name, tag in re.findall(r'UE_DEFINE_GAMEPLAY_TAG(?:_COMMENT|_STATIC)?\(\s*(\w+)\s*,\s*"([^"]+)"', text):
            out.setdefault(tag, []).append(name)
    return out


NATIVE = {}


def source_lines(tag):
    """The C++ and config lines that name the tag, as a string or by its native declaration's name: each as its file
    and its text, without a line number (which any edit above it would move)."""
    if not NATIVE:
        NATIVE.update(native_names() or {"": []})
    needle = f'"{tag}"'
    idents = [re.compile(r"\b" + re.escape(n) + r"\b") for n in NATIVE.get(tag, [])]
    hits = []
    for folder, globs in (("Source", ("*.h", "*.cpp")), ("Config", ("*.ini",))):
        for g in globs:
            for f in sorted((ROOT / folder).rglob(g)):
                if f.name == "DefaultGameplayTags.ini":
                    continue
                for line in f.read_text(encoding="utf-8", errors="replace").splitlines():
                    if needle in line or any(n.search(line) for n in idents) or (f.suffix == ".ini" and re.search(r"\b" + re.escape(tag) + r"\b", line)):
                        hits.append(f"{f.relative_to(ROOT).as_posix()}: {line.strip()[:160]}")
    return hits


def tag_table():
    rows = {}
    for tag in sorted(call_tool(TAGS, "ListTags", {"parentTag": ""})):
        info = call_tool(TAGS, "GetTagInfo", {"tagName": tag})
        family = next((f for f in sorted(FAMILY_OWNER, key=len, reverse=True) if tag == f or tag.startswith(f + ".")), None)
        rows[tag] = {"source": info.get("source"), "children": sorted(info.get("children") or []),
                     "assets": sorted(call_tool(TAGS, "FindReferencersByTag", {"tagName": tag}) or []),
                     "code": source_lines(tag), "owner": FAMILY_OWNER.get(family), "plugin_name": None, "step": None}
    return rows


def collect():
    result = {part: tool("dump_content", part=part) for part in PARTS}
    result["callers"] = callers(result["graphs"])
    result["tags"] = tag_table()
    return result


def suite(ctx):
    """Each part equal to its baseline."""
    mygame.use(ctx)
    result = collect()
    problems, files = [], {}
    for part, data in result.items():
        name = f"{part}.json"
        (ctx.out / name).write_text(json.dumps(data, indent=1, sort_keys=True))
        files[name] = f"Tests/baselines/dumps/{name}"
        base = BASE / name
        if not base.exists():
            problems.append(f"{part}: no baseline yet")
            continue
        expected = json.loads(base.read_text())
        changed = sorted(k for k in set(expected) | set(data) if expected.get(k) != data.get(k))
        if changed:
            problems.append(f"{part}: {len(changed)} entries differ: {', '.join(changed[:6])}{' ...' if len(changed) > 6 else ''}")
    for p in problems:
        print("  " + p)
    return {"ok": not problems, "checks": len(result), "problems": problems, "files": files}
