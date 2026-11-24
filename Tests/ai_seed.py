"""Step 0c item 5, the seeding try: does seeding the random make an enemy's AI repeat exactly at 1/60?

A manual suite (`unreal-agent test --only ai-seed`). Each run starts a fresh seed 6032 run, freezes the level's enemies,
spawns a fresh BP_Enemy1 (spawn_enemy, seeded first) facing the still player, then on the game clock seeds again and
wakes its AI, and records both characters every frame, unrounded (run_timeline's watch). Three scenarios: the player in
reach, 5 m away, and 5 m away walking. Seeds 77, 77, 77, 78 and two unseeded runs: the 77s must match frame for frame,
and 78 and the unseeded runs show that the AI draws at all.

Found (2026-11-16): it repeats exactly once both of the AI's randoms are seeded: the engine's global random (the tree's
stock nodes, Blueprint random nodes) and the AI system's own stream, which EQS's RandomBest25Pct pick draws from
(seed_random seeds both). The enemy must be spawned fresh: the level's enemies differ run to run on the same run seed
(which type stands at each spawn point is drawn unseeded), so one borrowed from the level brings its own history.
Found (2026-11-24): with the engine's animation in parallel (its default) a seed's runs still parted now and then, by
one frame, on a hit's frame: the attacker's next combo punch started in the hit's frame or the next. Every position and
draw matched up to it. The hit confirm comes from the attacker's anim work, which ran on worker threads, so it landed
before or after the pattern's next press by thread timing. With parallel animation off (this suite's launch arguments
in unreal-agent.toml) it repeated 4 runs of 4; before, about half failed.
CANARY = True draws and reseeds once a frame and records both streams, to find the frame where two runs' draws part.
"""
import json

import ability_regression as ar
import mygame
from mygame import tool, wait_until

RUNS = (77, 77, 77, 78, None, None)  # None: not seeded
SECONDS = 8.0
CANARY = False
# the player's stick on the game clock: [t, forward, right]
SCENARIOS = {"close_still": (95.0, []),
             "far_still": (500.0, []),
             "far_walking": (500.0, [[0.5, 0.0, 1.0], [1.5, 0.0, -1.0], [3.0, 0.0, 0.0], [4.0, -1.0, 0.0], [4.6, 0.0, 0.0]])}
ENEMY = "/Game/Blueprints/Chars/EMelee1/BP_Enemy1.BP_Enemy1_C"


def one_run(ctx, seed, gap, stick):
    """A fresh BP_Enemy1 spawned (seeded first) facing the still player, woken on the game clock after a second seed."""
    ctx.reset()
    player = tool("pie_player_state")
    player_path = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    ar.freeze_enemies()
    x, y = ar.DUMMY_SPOT
    tool("teleport_player", x=x - gap, y=y, z=ar.stand_z())
    tool("set_player_facing", yaw=0.0)
    for a, v in (("MaxHealth", 1000), ("Health", 1000), ("Defense", 1.0)):
        tool("set_player_attribute", attribute=a, value=v)
    if seed is not None:
        tool("reset_pattern_places", seed=seed)  # its pattern's salt from the seed, as in a fresh room of that run
    path = tool("spawn_enemy", class_path=ENEMY, x=x, y=y, z=ar.stand_z(), yaw=180.0, seed=seed if seed is not None else -1)["path"]
    mygame.wait_game(0.5)
    others = sorted([c["class"], [round(v, 1) for v in tool("character_state", actor_path=c["path"])["location"]]]
                    for c in tool("list_characters") if c["team"] == 1 and c["path"] != path and c.get("health", 0) > 0)
    inputs = ([[0.0, "call", "seed_random", {"seed": seed}]] if seed is not None else []) +              [[0.0, "call", "set_ai_enabled", {"actor_path": path, "enabled": True}]] +              [[t, "call", "move_input", {"forward": f, "right": r}] for t, f, r in stick]
    tool("run_timeline", spec=json.dumps({"dummy": path, "duration": SECONDS, "inputs": inputs, "settle": 0.0, "canary": CANARY,
                                          "watch": [path, player_path]}))
    ctx.wait_game(SECONDS + 0.3)  # one call, no polls while the AI runs
    r = wait_until(lambda: (t := tool("timeline_result"))["done"] and t, timeout=120, every=0.1, what="the AI timeline")
    tool("set_ai_enabled", actor_path=path, enabled=False)
    w = r["result"]["watch"]
    return {"seed": seed, "frames": len(w[path]), "dummy_rows": w[path], "player_rows": w[player_path],
            "canary": r["result"].get("canary", []), "others": others}


def first_difference(a, b):
    for side in ("canary", "dummy_rows", "player_rows"):
        for i, (x, y) in enumerate(zip(a[side], b[side])):
            if x != y:
                return f"{side} frame {i}: {x} vs {y}"
        if len(a[side]) != len(b[side]):
            return f"{side}: {len(a[side])} vs {len(b[side])} frames"
    return None


def suite(ctx):
    mygame.use(ctx)
    ctx.reset()
    lines, out = [f"budgets lifted: {tool('seed_random', seed=0)['lifted']}"], {}
    for name, (gap, stick) in SCENARIOS.items():
        runs = out[name] = [one_run(ctx, s, gap, stick) for s in RUNS]
        lines.append(f"== {name} (gap {gap})")
        lines += compare_runs(runs)
    (ctx.out / "ai_seed.json").write_text(json.dumps(out))
    for line in lines:
        print("  " + line)
    problems = []
    for name, runs in out.items():
        same = [r for r in runs if r["seed"] == RUNS[0]]
        problems += [f"{name}: seed {RUNS[0]} run {i} parts from run 0 at {d}" for i, r in enumerate(same[1:], 1)
                     if (d := first_difference(same[0], r))]
    if all(first_difference(runs[0], r) is None for runs in out.values() for r in runs if r["seed"] not in (RUNS[0], None)):
        problems.append("another seed gave the same runs everywhere: the scenarios draw nothing")
    return {"ok": not problems, "checks": len(out), "problems": problems, "notes": lines}


def compare_runs(runs):
    lines = []
    for i in range(len(runs)):
        for j in range(i + 1, len(runs)):
            d = first_difference(runs[i], runs[j])
            lines.append(f"run {j} (seed {runs[j]['seed']}) vs run {i} (seed {runs[i]['seed']}): {d or 'identical'}")
    for i, r in enumerate(runs):
        lines.append(f"run {i} others: {r['others']}")
        seq = [m for k, m in enumerate(x[5] for x in r["dummy_rows"]) if m and (k == 0 or r["dummy_rows"][k - 1][5] != m)]
        first = next((x[0] for x in r["dummy_rows"] if x[5]), None)
        lines.append(f"run {i} (seed {r['seed']}): first attack at {first}, {len(seq)} montages, player health at end "
                     f"{r['player_rows'][-1][6]}, dummy ends at {[round(v, 1) for v in r['dummy_rows'][-1][1:4]]}")
    return lines
