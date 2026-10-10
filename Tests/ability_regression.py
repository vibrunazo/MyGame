"""Ability regression run for Phase 3: presses the player's buttons in PIE and records what happens.

Needs the editor running with PIE started (see MIGRATION.md "How we test"). Every enemy is frozen (AI and
sensing off); one is brought into the start room as a still dummy in front of the player (Defense 1000).

    python Tests/ability_regression.py run  out.json        # record
    python Tests/ability_regression.py diff Tests/baselines/abilities_phase3.json out.json

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
        time.sleep(0.4)
        px, py, _ = tool("pie_player_state")["location"]
        dx, dy, _ = tool("character_state", actor_path=dummy["path"])["location"]
        if abs(px - player_x) < 30 and abs(py - y) < 30 and abs(dx - x) < 30 and abs(dy - y) < 30:
            return


def reset(player, dummy):
    for b in ("Punch", "Kick", "Cast", "Jump", "Super", "Ultra"):
        tool("press_input", button=b, pressed=False)
    tool("move_input", forward=0.0, right=0.0)
    freeze_enemies()
    time.sleep(1.5)
    for a, v in (("MaxHealth", 1000), ("Health", 1000), ("Defense", 1000), ("MaxMana", 100), ("Mana", 100)):
        tool("set_player_attribute", attribute=a, value=v)
    for a in ("MaxHealth", "Health"):
        tool("set_character_attribute", actor_path=dummy["path"], attribute=a, value=1000.0)
    tool("set_character_attribute", actor_path=dummy["path"], attribute="Defense", value=dummy["defense"])
    place(dummy, DUMMY_SPOT[0] - 95)


def run_scenario(name, actions, duration, player, dummy, away_from_dummy=False):
    reset(player, dummy)
    if away_from_dummy:  # movement: start further back so walking and dashing (along -X) have room
        place(dummy, START_ROOM_SPOT[0])
    start_dummy = tool("character_state", actor_path=dummy["path"])
    start_player = tool("pie_player_state")
    timeline, seen, dummy_tags, dummy_z, player_z = [], set(), set(), start_dummy["location"][2], start_player["location"][2]
    player_tags = set()
    pending = sorted(actions, key=lambda a: a[0])
    t0 = time.time()
    while time.time() - t0 < duration:
        now = time.time() - t0
        # at most one input per sample, so the game ticks between presses (the editor can answer several
        # tool calls in one frame; two presses landing together would be one simultaneous press)
        if pending and pending[0][0] <= now:
            pending.pop(0)[1]()
            time.sleep(0.05)
        anim = tool("player_anim")
        key = f"{anim['montage']}:{anim['section']}" if anim["montage"] else None
        if key and (not timeline or timeline[-1] != key):
            timeline.append(key)
        seen.update(active_abilities(player))
        d = tool("character_state", actor_path=dummy["path"])
        dummy_tags.update(t for t in active_tags(dummy["path"]) if t.startswith(("status.", "state.")))
        dummy_z = max(dummy_z, d["location"][2])
        ps = tool("pie_player_state")
        player_z = max(player_z, ps["location"][2])
        # state.combat is a timed buff that can outlast the previous scenario: noise
        player_tags.update(t for t in active_tags(player) if t.startswith(("state.", "status.")) and t != "state.combat")
        time.sleep(SAMPLE)
    end_dummy = tool("character_state", actor_path=dummy["path"])
    end_player = tool("pie_player_state")
    # a combo window must close with the attack (combo.cancancel left behind lets any attack cancel any other):
    # look once the player's abilities have ended
    t1 = time.time()
    while active_abilities(player) and time.time() - t1 < 3.0:
        time.sleep(0.1)
    lingering = sorted(t for t in active_tags(player) if t.startswith("combo."))
    gap = [round(a - b, 1) for a, b in zip(start_dummy["location"], start_player["location"])]
    return {"scenario": name, "lingering": lingering, "start_gap": gap, "start_yaw": [start_player["yaw"], start_player["control_yaw"]],
            "timeline": timeline, "abilities": sorted(seen),
            "dummy_damage": round(start_dummy["health"] - end_dummy["health"], 1),
            "dummy_tags": sorted(dummy_tags), "dummy_rise": round(dummy_z - start_dummy["location"][2], 1),
            "dummy_pushed": round(sum((a - b) ** 2 for a, b in zip(end_dummy["location"][:2], start_dummy["location"][:2])) ** 0.5, 1),
            "player_mana": round(end_player["attributes"]["Mana"] - start_player["attributes"]["Mana"], 1),
            "player_tags": sorted(player_tags),
            "player_moved": round(sum((a - b) ** 2 for a, b in zip(end_player["location"][:2], start_player["location"][:2])) ** 0.5, 1),
            "player_rise": round(player_z - start_player["location"][2], 1)}


def press(button, down=True):
    return lambda: tool("press_input", button=button, pressed=down)


def tap(button, at, length=0.06):
    """Press and release as one action, so the hold lasts `length` and not a whole sampling cycle."""
    def act():
        tool("press_input", button=button, pressed=True)
        time.sleep(length)
        tool("press_input", button=button, pressed=False)
    return [(at, act)]


def move(forward, right=0.0):
    return lambda: tool("move_input", forward=forward, right=right)


def double_tap(forward):
    """Two quick stick flicks in one action, so their spacing is real time and not sampling cycles."""
    def act():
        for _ in range(2):
            tool("move_input", forward=forward, right=0.0)
            time.sleep(0.04)
            tool("move_input", forward=0.0, right=0.0)
            time.sleep(0.03)
    return act


def learn(item):
    return lambda: tool("learn_item_abilities", item_asset_path=ITEMS + item)


def scenarios():
    yield "punch_tap", tap("Punch", 0.0), 1.5
    yield "punch_hold", [(0.0, press("Punch")), (2.4, press("Punch", False))], 3.2
    yield "kick_tap", tap("Kick", 0.0), 1.5
    yield "kick_hold", [(0.0, press("Kick")), (2.4, press("Kick", False))], 3.2
    yield "jump_kick", tap("Jump", 0.0, 0.2) + tap("Kick", 0.25), 1.8
    yield "jump_punch", tap("Jump", 0.0, 0.2) + tap("Punch", 0.25), 1.8
    yield "dash", [(0.0, lambda: tool("dash_player"))], 1.2, True
    # movement through IA_Move (away from the dummy, along -X)
    yield "walk_back", [(0.0, move(-1.0)), (1.0, move(0.0))], 1.4, True
    yield "dash_double_tap", [(0.0, double_tap(-1.0))], 1.2, True
    for item, button in (("DA_LearnSmash", "Punch"), ("DA_LearnRunPunch", "Punch"), ("DA_LearnUppercut", "Punch"),
                         ("DA_LearnTat", "Kick"), ("DA_LearnKkk", "Kick")):
        yield (f"super_{button.lower()}_{item[8:].lower()}",
               [(0.0, learn(item)), (0.1, press("Super"))] + tap(button, 0.2) + [(0.5, press("Super", False))], 2.2)
    yield "cast_fireball", [(0.0, learn("DA_LearnFireball"))] + tap("Cast", 0.2), 1.8
    # punch right after the super move: plain punch still works when Super isn't held
    yield "punch_after_learning", tap("Punch", 0.0), 1.5
    # damage path: a punch against Defense 2 (the exec scales by Attack / Defense), and Fire Hands' damage over time
    yield "punch_vs_defense2", [(0.0, set_dummy("Defense", 2.0))] + tap("Punch", 0.2), 1.5
    # a learned Smash must not fire by itself when health crosses 75% (it used to carry the boss's triggers)
    yield "health_75_with_smash", [(0.0, learn("DA_LearnSmash")), (0.2, set_player_health(700.0))], 2.0
    # last: the item stays in the inventory and the burn outlasts the reset
    yield "fire_hands_punch", [(0.0, lambda: tool("give_item", item_asset_path=ITEMS + "DA_ItemFireHands"))] + tap("Punch", 0.2), 6.0


def set_dummy(attribute, value):
    return lambda: tool("set_character_attribute", actor_path=DUMMY[0]["path"], attribute=attribute, value=value)


DUMMY = [None]


def set_player_health(value):
    def act():
        tool("set_player_attribute", attribute="Health", value=value)
        tool("refresh_health", actor_path=PLAYER_PATH[0])
    return act


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


def run(out):
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
    with open(out, "w") as f:
        json.dump(results, f, indent=1)


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
        montages, effects, t0 = set(), set(), time.time()
        while time.time() - t0 < seconds:
            m = tool("character_state", actor_path=boss["path"])["montage"]
            if m: montages.add(m)
            for e in call_tool(GAS, "GetActiveEffects", {"actor": {"refPath": boss["path"]}}):
                effects.add(str(e.get("effectName", e))[:60] if isinstance(e, dict) else str(e)[:60])
            time.sleep(0.1)
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
        time.sleep(0.3)
        tool("set_ai_enabled", actor_path=dummy["path"], enabled=True, show_player=False)
        t0, seen = time.time(), None
        while time.time() - t0 < seconds and not seen:
            seen = tool("ai_target", actor_path=dummy["path"])["target"]
            time.sleep(0.1)
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
            "hit": s["dummy_damage"] > 0, "launched": s["dummy_rise"] > 20, "pushed": s.get("dummy_pushed", 0) > 10,
            "mana": s["player_mana"],
            "jumped": s["player_rise"] > 50, "moved": s.get("player_moved", 0) > 100,
            "player_tags": s.get("player_tags"), "dummy_tags": s.get("dummy_tags"),
            "damage": None if s["scenario"] in VARIABLE_DAMAGE else s["dummy_damage"],
            "lingering": s.get("lingering")}


# held combos and multi-hit moves land a varying number of hits; everything else deals the same damage every run
VARIABLE_DAMAGE = {"punch_hold", "kick_hold", "super_kick_tat"}


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
            if oa[k] == ob[k]:
                continue
            if k.endswith("_tags") and set(ob[k] or []) <= set(oa[k] or []):
                # brief tags (a hitstun) can fall between samples: a missing one is a note, a new one a difference
                print(f"note {name}.{k}: {oa[k]} -> {ob[k]}")
                continue
            print(f"{name}.{k}: {oa[k]} -> {ob[k]}")
            same = False
    if A.get("boss") and B.get("boss") and A["boss"] != B["boss"]:
        print(f"boss: {A['boss']} -> {B['boss']}")
        same = False
    if A.get("sight") and B.get("sight") and A["sight"] != B["sight"]:
        print(f"sight: {A['sight']} -> {B['sight']}")
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
