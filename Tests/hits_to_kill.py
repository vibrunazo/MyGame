"""Ass Step 11's gate table: hits to kill, per player ability per enemy.

The `hits-to-kill` suite of `unreal-agent test` (`suite(ctx)`). For each enemy type, a fresh one is spawned as the
abilities suite's dummy (its own Defense; Max Health and Health raised to 1000 so one use never kills it), and each
player move whose damage is the same every run (ability_regression's scenarios, VARIABLE_DAMAGE and the movement ones
left out) is played once against it, in a fresh run for each enemy (the moves learn items). Per enemy and move: the damage one use deals, and the uses it takes to kill that
enemy from its own Max Health. Compared exactly with Tests/baselines/hits_to_kill.json, recorded before the move to the
plugin's attributes and rules (Step 11), so the move must keep every number.
"""
import json
import math
from pathlib import Path

import mygame
from mygame import objects, tool
import ability_regression as ar
import strict

BASE = Path(__file__).resolve().parent / "baselines" / "hits_to_kill.json"
KINDS = ("enemy1", "enemy2", "enemy3", "kicker", "wizard", "boss")
SKIP = ar.VARIABLE_DAMAGE | {"dash", "walk_back", "dash_double_tap", "health_75_with_smash"}


def table(ctx):
    out = {}
    for kind in KINDS:
        # a fresh run per enemy: the moves learn items (Fire Hands stays in the inventory and burns every later hit)
        ctx.reset()
        player = tool("pie_player_state")
        ar.PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
        path = tool("spawn_enemy", class_path=strict.CHARS[kind], x=ar.DUMMY_SPOT[0], y=ar.DUMMY_SPOT[1], z=ar.stand_z(),
                    yaw=180.0, frozen=True)["path"]
        max_health = objects(path, ["MaxHealth"])[0]["properties"]["MaxHealth"]
        dummy = {"path": path, "defense": tool("character_state", actor_path=path)["defense"]}
        ar.DUMMY[0] = dummy
        row = {}
        for name, inputs, duration, *start_room in ar.scenarios():
            if name in SKIP:
                continue
            damage = ar.run_scenario(name, inputs, duration, ar.PLAYER_PATH[0], dummy, bool(start_room))["dummy_damage"]
            row[name] = {"damage": damage, "hits_to_kill": math.ceil(max_health / damage) if damage > 0 else None}
        out[kind] = {"max_health": max_health, "defense": dummy["defense"], "moves": row}
        tool("destroy_actor", actor_path=path)
    return out


def suite(ctx):
    mygame.use(ctx)
    result = table(ctx)
    (ctx.out / "hits_to_kill.json").write_text(json.dumps(result, indent=1))
    for kind, r in result.items():
        print(f"  {kind} (max health {r['max_health']}, defense {r['defense']}): " +
              ", ".join(f"{m} {v['damage']}/{v['hits_to_kill']}" for m, v in r["moves"].items()))
    problems = []
    if BASE.exists():
        expected = json.loads(BASE.read_text())
        for kind in sorted(set(expected) | set(result)):
            if expected.get(kind) != result.get(kind):
                a, b = expected.get(kind, {}), result.get(kind, {})
                diffs = [k for k in sorted(set(a) | set(b)) if a.get(k) != b.get(k)]
                moves = a.get("moves", {}), b.get("moves", {})
                diffs += [f"{m}: {moves[0].get(m)} -> {moves[1].get(m)}" for m in sorted(set(moves[0]) | set(moves[1]))
                          if moves[0].get(m) != moves[1].get(m)]
                problems.append(f"{kind}: {'; '.join(str(d) for d in diffs)}")
    else:
        problems.append("no baseline yet (Tests/baselines/hits_to_kill.json)")
    return {"ok": not problems, "checks": len(result), "problems": problems,
            "files": {"hits_to_kill.json": "Tests/baselines/hits_to_kill.json"}}
