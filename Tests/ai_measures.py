"""Step 0c item 4: the AI measures, and the tolerances Step 7 is held to (Ass Plan.md, Step 0c and 1.1).

Fixed here, before any number is computed and before anyone sees the port (the rule committed on its own first; its
second form, below, agreed with the plan's session on 2026-11-17, still before any number).

What chance decides (which spot the query picks, when a wait ends against the player's movement) is measured over many
seeded runs; the decision matrix and the boss scenarios (ai_behaviour.py) carry what is exact.

RUNS
  Solo: each enemy type (TYPES) alone against the player, in each player behaviour (BEHAVIOURS), for each seed in
  SEEDS, SOLO_SECONDS long. Group: the mixed group GROUP (three enemies at once), in each behaviour, for each seed,
  GROUP_SECONDS long. Each run spawns its enemies fresh at the same spots, wakes them with the player as their target,
  and is seeded (run_timeline's seed: every tick reseeded from the seed and the tick's number). The player has health
  and defense enough never to die, never attacks, and is kept inside BOUNDS, a box
  well inside the start room, as by a wall: the group's knockbacks carried it out through the room's opening, which
  ended the play session.

BEHAVIOURS (the player's)
  still     stands facing the enemies' side
  away      stands with its back to them
  walking   walks back and forth across the room, 2 s each way
  circling  walks a square, 1.5 s a side

MEASURES (per run; then per cell, a cell being type x behaviour, or the group x behaviour)
  first_attack   seconds from waking to the enemy's first ability start (the run's length if none)
  behind_share   of its ability starts, the share begun with it behind the player (more than 90 degrees from the
                 player's facing)
  longest_gap    the longest time without any enemy starting an ability, in seconds
  distance       the mean distance between the enemy and the player, in cm
  per_minute     its ability starts per minute (per enemy in the group)
  retreat        the wizard only: the mean of its distance change over the 0.5 s after the player comes within its
                 aggressive range (positive: it backs away)
  at_once        group only: the mean over frames of how many enemies are in an ability at once
  token_changes  group only: how many times the player's attack token changes hands: changes of the set of enemies on
                 its aggro list (the list collects the same holder many times over: the tree's TagTarget task adds it
                 every time it runs, and the game doesn't check; counting the list's changes counted those adds,
                 about 90 in 15 s with no change of holder; found before the reference, 2026-11-17)

How long a stunned holder keeps the token is not measured here but exact, in ai_behaviour.py (token_stunned_holder):
the group's player never attacks, and a player punching every 0.8 s (tried before the reference) hardly reached the
holder, the kicker keeping its range.

THE TOLERANCE RULE (Step 7 against this reference)
  For each cell and measure, a sample of runs gives a mean m and a standard deviation s over its n runs (a run without
  the measure, such as no attack to share, is left out). Sample B passes against sample A when all three hold:
    |mB - mA| <= max(K * sqrt(sA^2 / nA + sB^2 / nB), FLOOR[measure])
    sB <= 2 * sA + FLOOR[measure]           (not more erratic)
    sB >= sA / 2 - FLOOR[measure]           (not more uniform: a pattern that always picks the same spot or timing)
  with K = 3. The reference keeps REFERENCE_SEEDS (20) runs per cell.
  A/A self-test, when the reference is taken: today's tree on seeds 1-10 against itself on seeds 11-20, under this same
  rule, must pass every cell. If one fails, that measure's floor (or K) is widened and the reason written here, before
  Step 7 exists. Then the reference is recorded in a second session too, and must pass against the first.
  Since Step 7 every enemy runs its pattern, and this suite is its gate (agreed 2026-11-19): every cell on seeds 1-20
  against the reference (the pattern draws differently, so the seeds don't pair its runs with the tree's), and the A/A;
  a cell outside the A/A runs seeds 21-40, and passes if seeds 1-20 and 21-40 are within the rule. One rare mode (enemy3 circling: 18 attacks a minute, a 6 s gap,
  in 2 of the tree's 20 reference runs) falls in one half of 20 by chance.
  Reference videos: one run per type and behaviour (seed 1), kept beside the reference for the user to compare by eye
  with the port's.

Who takes the token when two enemies qualify on one tick is exact, not measured: a scenario puts two enemies in range
on the same tick, run in both orders of the recorder's roles (it orders their ticks), and records who holds the token.
"""
import json
import math
import statistics
from pathlib import Path

import mygame
from mygame import tool
import ability_regression as ar
import strict
from strict import scenario, enemy, at, move, tap, Y

TYPES = ("enemy1", "enemy2", "enemy3", "kicker", "wizard")
GROUP = ("enemy1", "enemy2", "kicker")
BEHAVIOURS = ("still", "away", "walking", "circling")
REFERENCE_SEEDS = tuple(range(1, 21))
SEEDS = REFERENCE_SEEDS
SOLO_SECONDS = 10.0
GROUP_SECONDS = 15.0
K = 3.0
FLOOR = {"first_attack": 0.25, "behind_share": 0.1, "longest_gap": 0.5, "distance": 25.0, "per_minute": 3.0,
         "retreat": 25.0, "at_once": 0.25, "token_changes": 1.0}

ROOT = Path(__file__).resolve().parent.parent
REFERENCE = ROOT / "Saved" / "AssReference" / "ai_measures.json"
PLAYER_START = (0.0, Y)
SOLO_SPOT = [(450.0, Y)]
GROUP_SPOTS = [(450.0, Y), (400.0, Y + 150.0), (400.0, Y - 150.0)]
WIZARD_AGGRESSIVE = 550.0
BOUNDS = [-650.0, 650.0, -950.0, 500.0]


def behaviour_inputs(behaviour, seconds):
    if behaviour == "still":
        return [at(0.0, "set_player_facing", yaw=0.0)]
    if behaviour == "away":
        return [at(0.0, "set_player_facing", yaw=180.0)]
    out, t, k = [], 0.0, 0
    if behaviour == "walking":
        while t < seconds:
            out += move(t, 1.0 if k % 2 == 0 else -1.0)
            t, k = t + 2.0, k + 1
    else:  # circling: a square
        sides = [(1.0, 0.0), (0.0, 1.0), (-1.0, 0.0), (0.0, -1.0)]
        while t < seconds:
            out += move(t, *sides[k % 4])
            t, k = t + 1.5, k + 1
    return out + move(seconds, 0.0)


def runs(seeds=SEEDS):
    """(cell, seed, scenario, enemy roles, the first type): solo cells, then the group."""
    cells = [(kind, (kind,), SOLO_SPOT, SOLO_SECONDS) for kind in TYPES] + [("group", GROUP, GROUP_SPOTS, GROUP_SECONDS)]
    for name, kinds, spots, seconds in cells:
        for behaviour in BEHAVIOURS:
            for seed in seeds:
                roles = ["enemy"] if len(kinds) == 1 else [f"enemy{i}" for i in range(len(kinds))]
                enemies = [enemy(r, k, x=x, y=y) for r, k, (x, y) in zip(roles, kinds, spots)]
                inputs = [at(0.0, "set_ai_enabled", actor_path="@" + r, enabled=True) for r in roles] + behaviour_inputs(behaviour, seconds)
                yield (f"{name}.{behaviour}", seed,
                       scenario(f"{name}.{behaviour}.{seed}", inputs, seconds, enemies=enemies, player_x=PLAYER_START[0],
                                player_y=PLAYER_START[1], player={"Defense": 1000.0}, wait=0.5, seed=seed, bounds=BOUNDS),
                       roles, kinds[0] if len(kinds) == 1 else "group")


def measure(record, roles, kind, seconds):
    """The measures of one run."""
    frames = {r: {f[0]: f for f in record["frames"].get(r, [])} for r in roles + ["player"]}
    starts = [(e[0], e[1], e[3]) for e in record["events"] if e[2] == "ability" and e[4] == "activated" and e[3] in roles]
    out = {"first_attack": min((t for _, t, _ in starts), default=seconds)}

    def behind(frame, role):
        p, e = frames["player"].get(frame), frames[role].get(frame)
        if not p or not e:
            return False
        yaw = math.radians(p[6])
        return (e[2] - p[2]) * math.cos(yaw) + (e[3] - p[3]) * math.sin(yaw) < 0.0

    out["behind_share"] = sum(behind(f, r) for f, _, r in starts) / len(starts) if starts else None
    times = sorted([0.0] + [t for _, t, _ in starts] + [seconds])
    out["longest_gap"] = max(b - a for a, b in zip(times, times[1:]))
    dists = [math.dist(frames[r][f][2:4], p[2:4]) for f, p in frames["player"].items() for r in roles if f in frames[r]]
    out["distance"] = sum(dists) / len(dists) if dists else None
    out["per_minute"] = len(starts) / seconds * 60.0 / len(roles)
    if kind == "wizard":
        d = {f: math.dist(row[2:4], frames["player"][f][2:4]) for f, row in frames["enemy"].items() if f in frames["player"]}
        keys = sorted(d)
        changes = [d[keys[i + 30]] - d[f] for i, f in enumerate(keys[:-30]) if i and d[keys[i - 1]] > WIZARD_AGGRESSIVE >= d[f]]
        out["retreat"] = sum(changes) / len(changes) if changes else None
    if kind == "group":
        by_frame = {}
        for e in record["events"]:
            by_frame.setdefault(e[0], []).append(e)
        active, busy = {r: 0 for r in roles}, []
        for f in sorted(frames["player"]):
            for e in by_frame.get(f, []):
                if e[2] == "ability" and e[3] in roles:
                    active[e[3]] += 1 if e[4] == "activated" else -1
            busy.append(sum(1 for r in roles if active[r] > 0))
        out["at_once"] = sum(busy) / len(busy) if busy else None
        holders = [frozenset(e[4]) for e in record["events"] if e[2] == "token" and e[3] == "player"]
        out["token_changes"] = sum(1 for a, b in zip(holders, holders[1:]) if a != b)
    return out


def cell_stats(per_run):
    """{measure: [mean, sd, n]} over a cell's runs."""
    stats = {}
    for m in FLOOR:
        values = [r[m] for r in per_run if r.get(m) is not None]
        if values:
            stats[m] = [statistics.fmean(values), statistics.stdev(values) if len(values) > 1 else 0.0, len(values)]
    return stats


def within(a_stats, b_stats):
    """Sample B against sample A under the rule of the header: problems, as lines."""
    problems = []
    for cell in sorted(set(a_stats) | set(b_stats)):
        for m in sorted(set(a_stats.get(cell, {})) | set(b_stats.get(cell, {}))):
            a, b = a_stats.get(cell, {}).get(m), b_stats.get(cell, {}).get(m)
            if not a or not b:
                problems.append(f"{cell}.{m}: {a} -> {b}")
                continue
            (ma, sa, na), (mb, sb, nb) = a, b
            band = max(K * math.sqrt(sa ** 2 / na + sb ** 2 / nb), FLOOR[m])
            if abs(mb - ma) > band or sb > 2.0 * sa + FLOOR[m] or sb < sa / 2.0 - FLOOR[m]:
                problems.append(f"{cell}.{m}: mean {ma:.3f} sd {sa:.3f} n {na} -> mean {mb:.3f} sd {sb:.3f} n {nb} (band {band:.3f})")
    return problems


def suite(ctx):
    """Every run and its measures; the A/A self-test (seeds 1-10 against 11-20); the rule against the UE4 reference
    when there is one."""
    mygame.use(ctx)
    ctx.reset()
    player = tool("pie_player_state")
    ar.PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    base = strict.base_attributes(player)
    per_run = {}
    for cell, seed, sc, roles, kind in runs():
        per_run.setdefault(cell, {})[seed] = measure(strict.run(sc, base), roles, kind, sc["duration"])
    stats = {cell: cell_stats(list(v.values())) for cell, v in per_run.items()}
    halves = [{cell: cell_stats([v[s] for s in v if s in part]) for cell, v in per_run.items()}
              for part in (range(1, 11), range(11, 21))]
    (ctx.out / "ai_measures.json").write_text(json.dumps(stats, indent=1, sort_keys=True))
    (ctx.out / "ai_runs.json").write_text(json.dumps(per_run, sort_keys=True))
    for cell, s in sorted(stats.items()):
        print(f"  {cell}: " + ", ".join(f"{m} {v[0]:.2f}±{v[1]:.2f}" for m, v in sorted(s.items())))
    aa = within(halves[0], halves[1])
    failing = {".".join(p.split(".")[:2]) for p in aa if ":" in p}
    if failing:
        # the rule's own procedure (Ass Step 7, 2026-11-19): a cell outside it runs 20 more seeds (21-40), and its A/A
        # is then seeds 1-20 against 21-40 (a rare mode, which the tree has too, can fall in one half of 20)
        print(f"  A/A outside the rule in {sorted(failing)}: seeds 21-40 there")
        for cell, seed, sc, roles, kind in runs(range(21, 41)):
            if cell in failing:
                per_run[cell][seed] = measure(strict.run(sc, base), roles, kind, sc["duration"])
        wide = [{cell: cell_stats([per_run[cell][s] for s in part]) for cell in failing}
                for part in (range(1, 21), range(21, 41))]
        aa = [p for p in aa if ".".join(p.split(".")[:2]) not in failing] + within(wide[0], wide[1])
        (ctx.out / "ai_runs.json").write_text(json.dumps(per_run, sort_keys=True))
    problems = [f"A/A {p}" for p in aa]
    if REFERENCE.exists():
        problems += within(json.loads(REFERENCE.read_text()), stats)
    for p in problems:
        print("  " + p)
    return {"ok": not problems, "checks": len(stats), "problems": problems, "files": {}}
