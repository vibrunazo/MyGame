"""Ability regression run for Phase 3: presses the player's buttons in PIE and records what happens.

Needs the editor running with PIE started (see MIGRATION.md "How we test"). An enemy from another room
is frozen (AI stopped) and used as a still dummy in front of the player; the player gets Defense 1000.

    python Tests/ability_regression.py run  out.json        # record
    python Tests/ability_regression.py diff before.json after.json

Per scenario it records the montage/section timeline, abilities seen active, the dummy's health
change, tags and highest Z (launches), and the player's mana change and highest Z.
"""
import json
import sys
import time

from mcp_client import call_tool, tool

GAS = "GASToolsets.AbilitySystemInspectorToolset"
ITEMS = "/Game/Blueprints/Items/"
SAMPLE = 0.08


def active_abilities(path):
    granted = call_tool(GAS, "GetGrantedAbilities", {"actor": {"refPath": path}})
    return sorted(a["abilityName"].removesuffix("_C") for a in granted if a["bIsActive"])


def reset(player, dummy):
    for b in ("Punch", "Kick", "Cast", "Jump", "Super", "Ultra"):
        tool("press_input", button=b, pressed=False)
    time.sleep(1.5)
    for a, v in (("MaxHealth", 1000), ("Health", 1000), ("Defense", 1000), ("MaxMana", 100), ("Mana", 100)):
        tool("set_player_attribute", attribute=a, value=v)
    for a in ("MaxHealth", "Health"):
        tool("set_character_attribute", actor_path=dummy["path"], attribute=a, value=1000.0)
    d = tool("character_state", actor_path=dummy["path"])
    x, y, z = d["location"]
    tool("teleport_player", x=x - 95, y=y, z=z + 5)
    tool("set_player_facing", yaw=0.0)
    time.sleep(0.4)


def run_scenario(name, actions, duration, player, dummy):
    reset(player, dummy)
    start_dummy = tool("character_state", actor_path=dummy["path"])
    start_player = tool("pie_player_state")
    timeline, seen, dummy_tags, dummy_z, player_z = [], set(), set(), start_dummy["location"][2], start_player["location"][2]
    pending = sorted(actions, key=lambda a: a[0])
    t0 = time.time()
    while time.time() - t0 < duration:
        now = time.time() - t0
        while pending and pending[0][0] <= now:
            pending.pop(0)[1]()
        anim = tool("player_anim")
        key = f"{anim['montage']}:{anim['section']}" if anim["montage"] else None
        if key and (not timeline or timeline[-1] != key):
            timeline.append(key)
        seen.update(active_abilities(player))
        d = tool("character_state", actor_path=dummy["path"])
        dummy_tags.update(t for t in d.get("tags", []) if t.startswith(("status.", "state.")))
        dummy_z = max(dummy_z, d["location"][2])
        player_z = max(player_z, tool("pie_player_state")["location"][2])
        time.sleep(SAMPLE)
    end_dummy = tool("character_state", actor_path=dummy["path"])
    end_player = tool("pie_player_state")
    return {"scenario": name, "timeline": timeline, "abilities": sorted(seen),
            "dummy_damage": round(start_dummy["health"] - end_dummy["health"], 1),
            "dummy_tags": sorted(dummy_tags), "dummy_rise": round(dummy_z - start_dummy["location"][2], 1),
            "player_mana": round(end_player["attributes"]["Mana"] - start_player["attributes"]["Mana"], 1),
            "player_rise": round(player_z - start_player["location"][2], 1)}


def press(button, down=True):
    return lambda: tool("press_input", button=button, pressed=down)


def tap(button, at, length=0.06):
    return [(at, press(button)), (at + length, press(button, False))]


def learn(item):
    return lambda: tool("learn_item_abilities", item_asset_path=ITEMS + item)


def scenarios():
    yield "punch_tap", tap("Punch", 0.0), 1.5
    yield "punch_hold", [(0.0, press("Punch")), (2.4, press("Punch", False))], 3.2
    yield "kick_tap", tap("Kick", 0.0), 1.5
    yield "kick_hold", [(0.0, press("Kick")), (2.4, press("Kick", False))], 3.2
    yield "jump_kick", tap("Jump", 0.0, 0.2) + tap("Kick", 0.25), 1.8
    yield "jump_punch", tap("Jump", 0.0, 0.2) + tap("Punch", 0.25), 1.8
    yield "dash", [(0.0, lambda: tool("activate_ability_event", event_name="dash"))], 1.2
    for item, button in (("DA_LearnSmash", "Punch"), ("DA_LearnRunPunch", "Punch"), ("DA_LearnUppercut", "Punch"),
                         ("DA_LearnTat", "Kick"), ("DA_LearnKkk", "Kick")):
        yield (f"super_{button.lower()}_{item[8:].lower()}",
               [(0.0, learn(item)), (0.1, press("Super"))] + tap(button, 0.2) + [(0.5, press("Super", False))], 2.2)
    yield "cast_fireball", [(0.0, learn("DA_LearnFireball"))] + tap("Cast", 0.2), 1.8
    # punch right after the super move: plain punch still works when Super isn't held
    yield "punch_after_learning", tap("Punch", 0.0), 1.5
    # does a learned Smash fire by itself when health crosses 75%? (GA_Smash has the boss's health triggers)
    yield "health_75_with_smash", [(0.0, learn("DA_LearnSmash")), (0.2, set_player_health(700.0))], 2.0


def set_player_health(value):
    def act():
        tool("set_player_attribute", attribute="Health", value=value)
        tool("refresh_health", actor_path=PLAYER_PATH[0])
    return act


PLAYER_PATH = [None]


def pick_dummy():
    chars = tool("list_characters")
    enemies = [c for c in chars if c["team"] == 1 and c.get("health", 0) > 0 and "Boss" not in c["class"]]
    if not enemies:
        raise RuntimeError("no living enemy to use as a dummy")
    dummy = sorted(enemies, key=lambda c: c["class"])[0]
    # every enemy stands still, so only the scenario's own inputs act (the layout is random per run)
    for c in chars:
        if c["team"] == 1 and c.get("health", 0) > 0:
            tool("set_ai_enabled", actor_path=c["path"], enabled=False)
    time.sleep(1.5)  # let them stop sliding
    return dummy


def run(out):
    player = tool("pie_player_state")
    player_path = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    PLAYER_PATH[0] = player_path
    dummy = pick_dummy()
    results = {"dummy": dummy["class"], "scenarios": []}
    for name, actions, duration in scenarios():
        r = run_scenario(name, actions, duration, player_path, dummy)
        print(json.dumps(r))
        results["scenarios"].append(r)
    results["enemy_attack"] = enemy_attack(dummy)
    print(json.dumps(results["enemy_attack"]))
    with open(out, "w") as f:
        json.dump(results, f, indent=1)


def enemy_attack(dummy, seconds=6.0):
    """Wakes the dummy's AI next to a vulnerable player: does it attack and land hits?"""
    reset(None, dummy)
    tool("set_player_attribute", attribute="Defense", value=1.0)
    tool("set_ai_enabled", actor_path=dummy["path"], enabled=True)
    start = tool("pie_player_state")["attributes"]["Health"]
    montages, t0 = [], time.time()
    while time.time() - t0 < seconds:
        m = tool("character_state", actor_path=dummy["path"])["montage"]
        if m and (not montages or montages[-1] != m):
            montages.append(m)
        time.sleep(SAMPLE)
    tool("set_ai_enabled", actor_path=dummy["path"], enabled=False)
    return {"enemy_montages": montages, "player_damage": round(start - tool("pie_player_state")["attributes"]["Health"], 1)}


def outcome(s):
    """What a scenario should keep doing across refactors; exact timings and damage totals vary run to run."""
    return {"montages": sorted({k.split(":")[0] for k in s["timeline"]}), "abilities": s["abilities"],
            "hit": s["dummy_damage"] > 0, "launched": s["dummy_rise"] > 20, "mana": s["player_mana"],
            "jumped": s["player_rise"] > 50}


def diff(a_path, b_path):
    A, B = json.load(open(a_path)), json.load(open(b_path))
    a = {s["scenario"]: s for s in A["scenarios"]}
    b = {s["scenario"]: s for s in B["scenarios"]}
    same = True
    for name in a:
        if name not in b:
            print(f"{name}: missing in {b_path}")
            same = False
            continue
        oa, ob = outcome(a[name]), outcome(b[name])
        for k in oa:
            if oa[k] != ob[k]:
                print(f"{name}.{k}: {oa[k]} -> {ob[k]}")
                same = False
    ea, eb = A.get("enemy_attack"), B.get("enemy_attack")
    if ea and eb and (set(ea["enemy_montages"]) != set(eb["enemy_montages"]) or (ea["player_damage"] > 0) != (eb["player_damage"] > 0)):
        print(f"enemy_attack: {ea} -> {eb}")
        same = False
    print("same outcomes" if same else "outcomes differ")


if __name__ == "__main__":
    if sys.argv[1] == "run":
        run(sys.argv[2])
    else:
        diff(sys.argv[2], sys.argv[3])
