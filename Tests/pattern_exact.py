"""Ass Step 7 item 6: the seeded pattern AI's exact record, and the rule it is compared by.

The `pattern-exact` suite of `unreal-agent test` (`suite(ctx)`): every ai-measures cell (ai_measures.runs) on seed 1, each
in a fresh world (a new seed 6032 run: no history from earlier scenarios) with its spawns seeded, as ai_seed does, and
the scenario's pattern salts reset to its seed (strict.run). Each record is compared with its baseline, and one group cell
runs twice in the session, the two records compared by the same rule: a seeded room gives one record.

THE RULE (agreed with the Ass session 2026-11-20)
  Events (decisions, abilities, effects, tags, tokens, notifies, hits) are exact in every cell, groups included. Two
  kinds of field are not:
  - A spawn's or a cue's point, in every cell, within strict.TOLERANCE_CM for its kind (10 cm): the level's props run
    on the world's clock (a pickup's UI spawned 0.0007 cm apart in two fresh worlds), and fresh worlds reach a scenario
    at different absolute times.
  - A probe's vector (hit shapes, hits, backslides) in a group cell, which is a position like the frames': reported,
    not gated. In a solo cell it is exact.
  An event that differs otherwise is a failure, never noise.
  Solo cells' frames are exact but for the yaw, within SOLO_YAW degrees: two fresh worlds once differed by 0.0035
  degrees on one frame of 600 and agreed on the next (2026-11-20), every position equal.
  Group cells' frames are reported (the largest position and yaw differences printed), not gated. Why: the crowd
  avoidance (UCrowdFollowingComponent, DetourCrowd) sums each agent's neighbours in its crowd slot order, and the
  level's enemies take their slots in the floor's tile streaming order, which changes from one world to the next; two
  fresh worlds part by 1e-7 cm from there, and the drift is chaotic and grows with time (5.4 cm and 2.4 degrees by 12 s
  of group.circling), so any fixed bound would be a guess a longer fight outgrows; the measures (ai_measures.py) gate
  group positions statistically. If a group cell's events ever differ, the fix is ordering the crowd's slots by spawn
  order (Plan.md Step 7, option (b)), never a wider bound.
"""
import json
from pathlib import Path

import mygame
from mygame import tool
import strict
import ability_regression as ar
import ai_measures as am

SEED = 1
SOLO_YAW = 0.01
TWICE = "group.circling"  # the cell run twice: a seeded room gives one record
BASE = Path(__file__).resolve().parent / "baselines" / "pattern_exact.json"
YAW = 6  # a frame row's yaw (strict's frame row: frame, time, x, y, z, pitch, yaw, roll, montage, ...)


def fresh(ctx, seed):
    """A new seed 6032 run (a fresh world), the player as strict wants it, the randoms seeded before any spawn."""
    ctx.reset()
    player = tool("pie_player_state")
    ar.PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    base = strict.base_attributes(player)
    tool("seed_random", seed=seed)
    return base


def record(ctx, sc):
    return strict.run(sc, fresh(ctx, sc.get("seed", SEED)))


def _event_same(x, y, group, worst):
    """Two events under the rule: exact, or only their vector moved: a spawn's or a cue's point within its kind's
    tolerance, a group cell's probe vector by any amount (reported in worst)."""
    if x == y:
        return True
    i = strict.VECTOR_AT.get(x[2]) if len(x) > 2 and isinstance(x[2], str) else None
    if i is None or (x[2] == "probe" and not group) or len(x) != len(y) or len(x) <= i:
        return False
    u, v = x[i], y[i]
    if not (isinstance(u, list) and isinstance(v, list) and len(u) == len(v) == 3) or x[:i] + x[i + 1:] != y[:i] + y[i + 1:]:
        return False
    name = x[4] if x[2] == "probe" else x[2]
    apart = strict._apart(u, v)
    worst[name] = max(worst.get(name, 0.0), apart)
    return x[2] == "probe" or apart <= strict.TOLERANCE_CM[name]


def _row_same(x, y, group, worst):
    """Two frame rows of one role under the rule (a group cell's are only reported); worst gathers the largest position
    and yaw differences."""
    if x == y:
        return True
    if len(x) == len(y) and len(x) > YAW:
        worst["yaw"] = max(worst.get("yaw", 0.0), abs((x[YAW] - y[YAW] + 180.0) % 360.0 - 180.0))
        worst["frame"] = max(worst.get("frame", 0.0), strict._apart(x[2:5], y[2:5]))
    if group:
        return True
    if len(x) != len(y) or x[:5] != y[:5] or x[5] != y[5] or x[7:] != y[7:]:
        return False
    return abs((x[YAW] - y[YAW] + 180.0) % 360.0 - 180.0) <= SOLO_YAW


def difference(a, b, group, worst=None):
    """Where two records of one cell part under the rule, or None."""
    worst = {} if worst is None else worst
    if a.get("roles") != b.get("roles"):
        return f"roles {a.get('roles')} -> {b.get('roles')}"
    # in the plugin's names (strict.step11_names: the baseline is from before Ass Step 11's move)
    ea = strict.by_role(strict.folded_bt(strict.step11_names(a["events"])))
    eb = strict.by_role(strict.folded_bt(strict.step11_names(b["events"])))
    bases = {}
    if len(ea) != len(eb):
        return f"{len(ea)} events -> {len(eb)}"
    for i, (x, y) in enumerate(zip(ea, eb)):
        if not (_event_same(x, y, group, worst) or strict.same_speed(x, y, bases)):
            return f"event {i}: {x} -> {y}"
    for role in sorted(set(a["frames"]) | set(b["frames"])):
        fa, fb = a["frames"].get(role, []), b["frames"].get(role, [])
        if len(fa) != len(fb) and not group:
            return f"{role}: {len(fa)} frames -> {len(fb)}"
        for x, y in zip(fa, fb):
            if not _row_same(x, y, group, worst):
                return f"{role} frame {x[0]}: {x} -> {y}"
    return None


def suite(ctx):
    mygame.use(ctx)
    tool("seed_random", seed=SEED)  # lifts the AI's wall-clock budgets too
    cells = {cell: (sc, roles) for cell, seed, sc, roles, kind in am.runs(seeds=(SEED,))}
    records, problems, worst = {}, [], {}
    for cell, (sc, roles) in cells.items():
        records[cell] = record(ctx, sc)
        print(f"  {cell}: {len(records[cell]['events'])} events")
    again = record(ctx, cells[TWICE][0])
    d = difference(records[TWICE], again, len(cells[TWICE][1]) > 1, worst)
    print(f"  {TWICE} twice: " + ("one record" if not d else f"parts: {d[:300]}"))
    if d:
        problems.append(f"{TWICE} run twice: {d[:400]}")
    (ctx.out / "pattern_exact.json").write_text(json.dumps(records))
    if BASE.exists():
        expected = json.loads(BASE.read_text())
        for cell, (sc, roles) in cells.items():
            if cell not in expected:
                problems.append(f"{cell}: not in the baseline")
                continue
            d = difference(expected[cell], records[cell], len(roles) > 1, worst)
            if d:
                problems.append(f"{cell}: {d[:400]}")
    else:
        problems.append("no baseline yet (Tests/baselines/pattern_exact.json): accept this run's after the gate")
    print("  largest differences: " + ", ".join(f"{k} {v:.4g}" for k, v in sorted(worst.items())))
    return {"ok": not problems, "checks": len(cells) + 1, "problems": problems,
            "files": {"pattern_exact.json": "Tests/baselines/pattern_exact.json"}}
