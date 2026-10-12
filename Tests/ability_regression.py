"""Ability regression run for Phase 3: presses the player's buttons in PIE and records what happens.

Needs the editor running with PIE started (see MIGRATION.md "How we test"). Every enemy is frozen (AI and
sensing off); one is brought into the start room as a still dummy in front of the player (Defense 1000).

    python Tests/ability_regression.py run  out.json        # record (or through Tests/run_all.py)
    python Tests/ability_regression.py diff Tests/baselines/abilities_phase3.json out.json

Per scenario it records the montage/section timeline, abilities seen active, the dummy's health
change, tags and highest Z (launches), and the player's mana change and highest Z.

Each scenario's inputs play on the game clock inside the editor (the run_timeline hook), which also records every
frame there, so a scenario behaves the same at any game speed (Tests/run_all.py --speed).
"""
import json
import sys
import time

from harness import game_time, wait_game
from mcp_client import call_tool, tool

GAS = "GASToolsets.AbilitySystemInspectorToolset"
ITEMS = "/Game/Blueprints/Items/"


def active_tags(path):
    return call_tool(GAS, "GetActiveTags", {"actor": {"refPath": path}})


def active_abilities(path):
    granted = call_tool(GAS, "GetGrantedAbilities", {"actor": {"refPath": path}})
    return sorted(a["abilityName"].removesuffix("_C") for a in granted if a["bIsActive"])


# Everything happens in the start room: empty, walled, and the y = -300 row has no books. Next to a random
# enemy the player could end up outside the room grid (the game then reloads the level) or walk off an edge.
DUMMY_SPOT = (300.0, -300.0)
START_ROOM_SPOT = (0.0, -300.0)
FLOOR = []


def floor_z():
    if not FLOOR:
        FLOOR.append(tool("floor_height", x=DUMMY_SPOT[0], y=DUMMY_SPOT[1], from_z=500.0)["z"] + 100.0)
    return FLOOR[0]


def place(dummy, player_x):
    """Dummy at DUMMY_SPOT, player at player_x facing +X (towards the dummy); retried if the game moves them."""
    x, y = DUMMY_SPOT
    for _ in range(4):
        tool("teleport_actor", actor_path=dummy["path"], x=x, y=y, z=floor_z())
        tool("teleport_player", x=player_x, y=y, z=floor_z())
        tool("set_player_facing", yaw=0.0)
        wait_game(0.4)
        px, py, _ = tool("pie_player_state")["location"]
        dx, dy, _ = tool("character_state", actor_path=dummy["path"])["location"]
        if abs(px - player_x) < 30 and abs(py - y) < 30 and abs(dx - x) < 30 and abs(dy - y) < 30:
            return


def reset(player, dummy):
    for b in ("Punch", "Kick", "Cast", "Jump", "Super", "Ultra"):
        tool("press_input", button=b, pressed=False)
    tool("move_input", forward=0.0, right=0.0)
    freeze_enemies()
    wait_game(1.5)
    for a, v in (("MaxHealth", 1000), ("Health", 1000), ("Defense", 1000), ("MaxMana", 100), ("Mana", 100)):
        tool("set_player_attribute", attribute=a, value=v)
    for a in ("MaxHealth", "Health"):
        tool("set_character_attribute", actor_path=dummy["path"], attribute=a, value=1000.0)
    tool("set_character_attribute", actor_path=dummy["path"], attribute="Defense", value=dummy["defense"])
    place(dummy, DUMMY_SPOT[0] - 95)


def run_scenario(name, inputs, duration, player, dummy, away_from_dummy=False):
    reset(player, dummy)
    if away_from_dummy:  # movement: start further back so walking and dashing (along -X) have room
        place(dummy, START_ROOM_SPOT[0])
    start_dummy = tool("character_state", actor_path=dummy["path"])
    start_player = tool("pie_player_state")
    tool("run_timeline", spec=json.dumps({"dummy": dummy["path"], "duration": duration, "inputs": inputs}))
    while not (r := tool("timeline_result"))["done"]:
        time.sleep(0.05)
    rec = r["result"]
    end_dummy, end_player = rec["end_dummy"], rec["end_player"]
    gap = [round(a - b, 1) for a, b in zip(start_dummy["location"], start_player["location"])]
    return {"scenario": name, "lingering": rec["lingering"], "start_gap": gap, "start_yaw": [start_player["yaw"], start_player["control_yaw"]],
            "timeline": rec["timeline"], "abilities": rec["abilities"],
            "dummy_damage": round(start_dummy["health"] - end_dummy["health"], 1),
            "dummy_tags": rec["dummy_tags"], "dummy_rise": round(rec["dummy_z"] - start_dummy["location"][2], 1),
            "dummy_pushed": round(sum((a - b) ** 2 for a, b in zip(end_dummy["location"][:2], start_dummy["location"][:2])) ** 0.5, 1),
            "player_mana": round(end_player["attributes"]["Mana"] - start_player["attributes"]["Mana"], 1),
            "player_tags": rec["player_tags"],
            "player_moved": round(sum((a - b) ** 2 for a, b in zip(end_player["location"][:2], start_player["location"][:2])) ** 0.5, 1),
            "player_rise": round(rec["player_z"] - start_player["location"][2], 1)}


# timeline inputs: [game time, "call", MyGameTools hook, kwargs]
def at(t, hook, **kwargs):
    return [t, "call", hook, kwargs]


def press(button, t, down=True):
    return [at(t, "press_input", button=button, pressed=down)]


def tap(button, t, length=0.06):
    return press(button, t) + press(button, t + length, False)


def move(t, forward, right=0.0):
    return [at(t, "move_input", forward=forward, right=right)]


def double_tap(t, forward):
    """Two quick stick flicks (the dash's double tap)."""
    return move(t, forward) + move(t + 0.04, 0.0) + move(t + 0.07, forward) + move(t + 0.11, 0.0)


def learn(t, item):
    return [at(t, "learn_item_abilities", item_asset_path=ITEMS + item)]


def scenarios():
    yield "punch_tap", tap("Punch", 0.0), 1.5
    yield "punch_hold", press("Punch", 0.0) + press("Punch", 2.4, False), 3.2
    yield "kick_tap", tap("Kick", 0.0), 1.5
    yield "kick_hold", press("Kick", 0.0) + press("Kick", 2.4, False), 3.2
    yield "jump_kick", tap("Jump", 0.0, 0.2) + tap("Kick", 0.25), 1.8
    yield "jump_punch", tap("Jump", 0.0, 0.2) + tap("Punch", 0.25), 1.8
    yield "dash", [at(0.0, "dash_player")], 1.2, True
    # movement through IA_Move (away from the dummy, along -X)
    yield "walk_back", move(0.0, -1.0) + move(1.0, 0.0), 1.4, True
    yield "dash_double_tap", double_tap(0.0, -1.0), 1.2, True
    for item, button in (("DA_LearnSmash", "Punch"), ("DA_LearnRunPunch", "Punch"), ("DA_LearnUppercut", "Punch"),
                         ("DA_LearnTat", "Kick"), ("DA_LearnKkk", "Kick")):
        yield (f"super_{button.lower()}_{item[8:].lower()}",
               learn(0.0, item) + press("Super", 0.1) + tap(button, 0.2) + press("Super", 0.5, False), 2.2)
    yield "cast_fireball", learn(0.0, "DA_LearnFireball") + tap("Cast", 0.2), 1.8
    # punch right after the super move: plain punch still works when Super isn't held
    yield "punch_after_learning", tap("Punch", 0.0), 1.5
    # damage path: a punch against Defense 2 (the exec scales by Attack / Defense)
    yield "punch_vs_defense2", [at(0.0, "set_character_attribute", actor_path=DUMMY[0]["path"], attribute="Defense", value=2.0)] + tap("Punch", 0.2), 1.5
    # a learned Smash must not fire by itself when health crosses 75% (it used to carry the boss's triggers)
    yield "health_75_with_smash", learn(0.0, "DA_LearnSmash") + [at(0.2, "set_player_attribute", attribute="Health", value=700.0),
                                                                 at(0.21, "refresh_health", actor_path=PLAYER_PATH[0])], 2.0
    # last: the item stays in the inventory and the burn outlasts the reset
    yield "fire_hands_punch", [at(0.0, "give_item", item_asset_path=ITEMS + "DA_ItemFireHands")] + tap("Punch", 0.2), 6.0


DUMMY = [None]
PLAYER_PATH = [None]


def freeze_enemies():
    """Every enemy stands still, so only the scenario's own inputs act (rooms spawn enemies as the player moves)."""
    for c in tool("list_characters"):
        if c["team"] == 1 and c.get("health", 0) > 0:
            tool("set_ai_enabled", actor_path=c["path"], enabled=False)


def pick_dummy():
    chars = tool("list_characters")
    enemies = [c for c in chars if c["team"] == 1 and c.get("health", 0) > 0 and "Boss" not in c["class"]]
    if not enemies:
        raise RuntimeError("no living enemy to use as a dummy")
    dummy = sorted(enemies, key=lambda c: c["class"])[0]
    freeze_enemies()
    dummy["defense"] = tool("character_state", actor_path=dummy["path"])["defense"]
    return dummy


def run_scenarios(names):
    """Only these scenarios (no boss, sight or enemy checks): {name: dummy damage}."""
    player = tool("pie_player_state")
    PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    dummy = pick_dummy()
    DUMMY[0] = dummy
    out = {}
    for name, inputs, duration, *start_room in scenarios():
        if name in names:
            out[name] = run_scenario(name, inputs, duration, PLAYER_PATH[0], dummy, bool(start_room))["dummy_damage"]
    return out


def run(out):
    results = run_all()
    with open(out, "w") as f:
        json.dump(results, f, indent=1)


def run_all():
    player = tool("pie_player_state")
    player_path = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    PLAYER_PATH[0] = player_path
    dummy = pick_dummy()
    DUMMY[0] = dummy
    results = {"dummy": dummy["class"], "scenarios": []}
    for name, actions, duration, *start_room in scenarios():
        r = run_scenario(name, actions, duration, player_path, dummy, bool(start_room))
        print(json.dumps(r))
        results["scenarios"].append(r)
    results["boss"] = boss_phases()
    print(json.dumps(results["boss"]))
    results["sight"] = sight_checks(dummy)
    print(json.dumps(results["sight"]))
    results["enemy_attack"] = enemy_attack(dummy)
    print(json.dumps(results["enemy_attack"]))
    return results


def boss_phases(seconds=4.0):
    """Drops the boss (AI frozen, the player as its target) below 75%, 50% and 25% health. Its phase moves are instant
    abilities, so each phase records their results: summoned characters, the boss's effects and its montages."""
    boss = next((c for c in tool("list_characters") if "Boss" in c["class"] and c.get("health", 0) > 0), None)
    if not boss:
        return {"error": "no boss"}
    tool("set_ai_enabled", actor_path=boss["path"], enabled=False)
    tool("ai_target", actor_path=boss["path"], set_player=True)
    tool("set_character_attribute", actor_path=boss["path"], attribute="MaxHealth", value=1000.0)
    tool("set_character_attribute", actor_path=boss["path"], attribute="Health", value=1000.0)
    tool("refresh_health", actor_path=boss["path"])
    phases = {}
    for pct in (74, 49, 24):
        before = len(tool("list_characters"))
        tool("set_character_attribute", actor_path=boss["path"], attribute="Health", value=pct * 10.0)
        tool("refresh_health", actor_path=boss["path"])
        montages, effects, t0 = set(), set(), game_time()
        while game_time() - t0 < seconds:
            m = tool("character_state", actor_path=boss["path"])["montage"]
            if m: montages.add(m)
            for e in call_tool(GAS, "GetActiveEffects", {"actor": {"refPath": boss["path"]}}):
                effects.add(str(e.get("effectName", e))[:60] if isinstance(e, dict) else str(e)[:60])
            time.sleep(0.02)
        phases[f"below_{pct + 1}"] = {"summoned": len(tool("list_characters")) - before, "montages": sorted(montages), "effects": sorted(effects)}
        freeze_enemies()  # summons
    return phases


def sight_checks(dummy, seconds=2.0):
    """Does a woken enemy notice the player on its own? In front at 500 (yes), facing away (no), in front at 900 (no)."""
    results = {}
    # the one that should notice goes last: noticing aggroes the room, which outlasts the reset
    for name, dummy_yaw, gap in (("behind_500", 0.0, 500.0), ("front_900", 180.0, 900.0), ("front_500", 180.0, 500.0)):
        reset(None, dummy)
        x, y = DUMMY_SPOT
        tool("teleport_actor", actor_path=dummy["path"], x=x, y=y, z=floor_z(), yaw=dummy_yaw)
        tool("teleport_player", x=x - gap, y=y, z=floor_z())
        tool("ai_target", actor_path=dummy["path"], clear=True)
        wait_game(0.3)
        tool("set_ai_enabled", actor_path=dummy["path"], enabled=True, show_player=False)
        t0, seen = game_time(), None
        while game_time() - t0 < seconds and not seen:
            seen = tool("ai_target", actor_path=dummy["path"])["target"]
            time.sleep(0.02)
        tool("set_ai_enabled", actor_path=dummy["path"], enabled=False)
        tool("ai_target", actor_path=dummy["path"], clear=True)
        results[name] = bool(seen)
    return results


def enemy_attack(dummy, seconds=6.0):
    """Wakes the dummy's AI next to a vulnerable player: does it attack and land hits?"""
    reset(None, dummy)
    tool("set_player_attribute", attribute="Defense", value=1.0)
    tool("set_ai_enabled", actor_path=dummy["path"], enabled=True)
    start = tool("pie_player_state")["attributes"]["Health"]
    montages, t0 = [], game_time()
    while game_time() - t0 < seconds:
        m = tool("character_state", actor_path=dummy["path"])["montage"]
        if m and (not montages or montages[-1] != m):
            montages.append(m)
        time.sleep(0.02)
    tool("set_ai_enabled", actor_path=dummy["path"], enabled=False)
    return {"enemy_montages": montages, "player_damage": round(start - tool("pie_player_state")["attributes"]["Health"], 1)}


def outcome(s):
    """What a scenario should keep doing across refactors; exact timings and damage totals vary run to run."""
    return {"montages": sorted({k.split(":")[0] for k in s["timeline"]}), "abilities": s["abilities"],
            "hit": s["dummy_damage"] > 0, "launched": s["dummy_rise"] > 20, "pushed": s.get("dummy_pushed", 0) > 10,
            "mana": s["player_mana"],
            "jumped": s["player_rise"] > 50, "moved": s.get("player_moved", 0) > 100,
            "player_tags": s.get("player_tags"), "dummy_tags": s.get("dummy_tags"),
            "damage": None if s["scenario"] in VARIABLE_DAMAGE else s["dummy_damage"],
            "lingering": s.get("lingering")}


# held combos and multi-hit moves land a varying number of hits; everything else deals the same damage every run
VARIABLE_DAMAGE = {"punch_hold", "kick_hold", "super_kick_tat"}


def compare(A, B):
    """Outcome differences between two runs, as lines; lines starting with "note" don't count as differences."""
    lines = []
    a = {x["scenario"]: x for x in A["scenarios"]}
    b = {x["scenario"]: x for x in B["scenarios"]}
    for name in a:
        if name not in b:
            lines.append(f"{name}: missing")
            continue
        oa, ob = outcome(a[name]), outcome(b[name])
        for k in oa:
            if oa[k] == ob[k]:
                continue
            if k.endswith("_tags") and set(ob[k] or []) <= set(oa[k] or []):
                # brief tags (a hitstun) can fall between samples: a missing one is a note, a new one a difference
                lines.append(f"note {name}.{k}: {oa[k]} -> {ob[k]}")
                continue
            lines.append(f"{name}.{k}: {oa[k]} -> {ob[k]}")
    if A.get("boss") and B.get("boss") and A["boss"] != B["boss"]:
        lines.append(f"boss: {A['boss']} -> {B['boss']}")
    if A.get("sight") and B.get("sight") and A["sight"] != B["sight"]:
        lines.append(f"sight: {A['sight']} -> {B['sight']}")
    ea, eb = A.get("enemy_attack"), B.get("enemy_attack")
    if ea and eb and (set(ea["enemy_montages"]) != set(eb["enemy_montages"]) or (ea["player_damage"] > 0) != (eb["player_damage"] > 0)):
        lines.append(f"enemy_attack: {ea} -> {eb}")
    return lines


def diff(a_path, b_path):
    lines = compare(json.load(open(a_path)), json.load(open(b_path)))
    for line in lines:
        print(line)
    print("same outcomes" if not any(not line.startswith("note") for line in lines) else "outcomes differ")


if __name__ == "__main__":
    if sys.argv[1] == "run":
        run(sys.argv[2])
    else:
        diff(sys.argv[2], sys.argv[3])
