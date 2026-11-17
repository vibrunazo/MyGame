"""Step 0c item 4: MyGame's AI behaviour on the current tree (Ass Plan.md, Step 0c and 1.1).

The `ai` suite of `unreal-agent test`. Its scenarios run with the AI on, each seeded (the per-tick reseed of
run_timeline), on fresh spawned enemies, recorded by the strict recorder (the `bt` events give the tree's tasks).

The boss scenarios (exact): the boss's AI on, its movement frozen (Speed 0: its tree still ticks, it can't walk), and
a real hit from the player that crosses its health marks (75, 50 and 25% of MaxHealth):
  boss_idle        the boss hasn't seen the player, who punches its back across 75%
  boss_mid_attack  the boss is punching the player when the player's punch crosses 75%
  boss_two_marks   one punch takes it from 52 to 22 of 100: across 50% and 25%
Each records which of its moves started, on which frame, and whether it was stunned.

The decision matrix (Plan.md 1.1): each enemy type in each of a fixed set of situations, and what it does in the next
second, its AI woken with the player as its target (sight is the strict suite's):
  facing   the player faces it, faces away (it is behind the player), or stands side on
  range    inside its attack range, between its attack and aggressive ranges, or beyond its aggressive range
  token    the player's attack token is free, already this enemy's, or another enemy's
  control  it has control, or a 0.6 s hitstun starts as it wakes
The first decision (an ability it starts, or a tree task it starts) is exact in kind and within one frame of the
reference; everything after it in that second is exact in kind and order, its frames not compared (Plan.md, Step 0c
item 4: a start one frame off changes how far a moving enemy walks), but for one decision that falls inside the second
in one recording and just outside it in the other.

How a recording is compared with the reference (Plan.md, Step 0c item 4, the terms of 2026-11-17): each character's
decisions (abilities started, ended or cancelled, its tree's tasks, its stuns, its health) equal and in the same order;
and each decision's frame, counted from what caused it, equal, except that a start the AI makes itself (an ability it
starts, a task its tree starts) may be one frame off. A start the AI makes can land a frame apart between runs (its
tree sets the key down in one frame; its tick starts the ability that frame or the next), which counting from frame 0
would carry into every later frame. A decision's cause is taken as whichever of these the frames agree with: the
scenario's start, the decision just before it (any character's), or for an ability's end, its own start. Step 7 is
held to the same.
"""
import json
import re
from pathlib import Path

import mygame
from mygame import tool
import ability_regression as ar
import strict
from strict import scenario, enemy, tap, at, target_player, X, Y, CLOSE

ROOT = Path(__file__).resolve().parent.parent
REFERENCE = ROOT / "Saved" / "AssReference" / "ai.json"
FROZEN = {"Speed": 0.0}  # its tree still ticks; it can't walk


def boss_scenarios():
    wake = [at(0.0, "set_ai_enabled", actor_path="@boss", enabled=True, show_player=False)]
    # the player behind the boss, which faces away and hasn't seen it
    yield scenario("boss_idle", wake + tap("Punch", 0.2), 4.0, enemies=[enemy("boss", "boss", x=CLOSE + 95, yaw=0.0, Health=755.0, Defense=1.0, **FROZEN)],
                   player={"Defense": 1000.0})
    # the boss targets the player and starts its punches; the player's punch lands inside them
    yield scenario("boss_mid_attack", [at(0.0, "set_ai_enabled", actor_path="@boss", enabled=True)] + tap("Punch", 0.5), 5.0,
                   enemies=[enemy("boss", "boss", x=X + 30, Health=755.0, Defense=1.0, **FROZEN)], player_x=CLOSE + 30 - 15,
                   player={"Defense": 1000.0}, setup=[target_player("boss")])
    yield scenario("boss_two_marks", wake + tap("Punch", 0.2), 4.0,
                   enemies=[enemy("boss", "boss", x=CLOSE + 95, yaw=0.0, MaxHealth=100.0, Health=52.0, Defense=1.0, **FROZEN)],
                   player={"Defense": 1000.0, "Attack": 2.0})


# each enemy type: its class and its controller's ranges (attack, aggressive; the dumps' actors.json)
TYPES = {"enemy1": (140, 200), "enemy2": (140, 200), "enemy3": (140, 200), "kicker": (230, 300), "wizard": (550, 550)}
FACING = {"toward": 0.0, "away": 180.0, "side": 90.0}
HITSTUN = "/Game/Abilities/GE_HitStun.GE_HitStun_C"


def matrix_scenarios():
    for kind, (attack, aggressive) in TYPES.items():
        ranges = {"attack": 0.7 * attack, "between": (attack + aggressive) / 2 if aggressive > attack else attack * 1.15,
                  "beyond": min(1.6 * aggressive, 900.0)}
        for facing, yaw in FACING.items():
            for rng, gap in ranges.items():
                for token in ("free", "mine", "other"):
                    for control in ("yes", "stunned"):
                        enemies = [enemy("enemy", kind)]
                        setup = [target_player("enemy")]
                        if token == "mine":
                            setup.append(["aggro_add", {"actor_path": "@player", "target_path": "@enemy"}])
                        if token == "other":
                            enemies.append(enemy("other", "enemy1", x=X + 300, y=Y + 400))
                            setup.append(["aggro_add", {"actor_path": "@player", "target_path": "@other"}])
                        inputs = []
                        if control == "stunned":
                            inputs.append(at(0.0, "apply_effect", actor_path="@enemy", effect_class_path=HITSTUN,
                                             set_by_caller=json.dumps({"data.hitstun": 0.6})))
                        inputs.append(at(0.0, "set_ai_enabled", actor_path="@enemy", enabled=True))
                        yield scenario(f"{kind}.{facing}.{rng}.{token}.{control}", inputs, 1.0, enemies=enemies,
                                       player_x=X - gap, player_yaw=yaw, setup=setup, wait=0.5)


def main_task(snapshot):
    """The main tree's (BT_AIBase's) task in a snapshot of the active tasks, or None."""
    found = re.findall(r"BT_AIBase::([^\]]+\])", snapshot)
    return found[-1] if found else None


def matrix_entry(record, role="enemy"):
    """{"first": [kind, detail, frame], "then": [[kind, detail], ...]} of the enemy's decisions in the second: its
    abilities started, ended or cancelled, its stuns, and the main tree's tasks it moves to (the bt events sample the
    active tasks once a frame, so a task shorter than a frame shows in one recording and not another: the background
    attack subtree's tasks are left to the abilities they start, and repeats of a main task are folded)."""
    ds = [d for d in decisions(record, {role}) if d[1] == role and d[2] in ("activated", "ended", "cancelled", "bt", "stun")]
    first = next((i for i, d in enumerate(ds) if d[2] == "activated" or (d[2] == "bt" and "::" in d[3])), None)
    if first is None:
        return {"first": None, "then": []}
    then, last_task = [], main_task(ds[first][3]) if ds[first][2] == "bt" else None
    for d in ds[first + 1:]:
        if d[2] == "bt":
            task = main_task(d[3])
            if task and task != last_task:
                then.append(["task", task])
            last_task = task or last_task
        else:
            then.append([d[2], d[3]])
    return {"first": [ds[first][2], ds[first][3], ds[first][0]], "then": then}


def matrix_difference(a, b):
    if (a["first"] is None) != (b["first"] is None):
        return f"first decision {a['first']} -> {b['first']}"
    if a["first"] and (a["first"][:2] != b["first"][:2] or abs(a["first"][2] - b["first"][2]) > 1):
        return f"first decision {a['first']} -> {b['first']}"
    short, long_ = sorted((a["then"], b["then"]), key=len)
    if long_[:len(short)] == short and len(long_) - len(short) <= 1:
        return None  # one decision at the second's end, inside one recording and just outside the other
    if a["then"] != b["then"]:
        i = next((i for i, (x, y) in enumerate(zip(a["then"], b["then"])) if x != y), min(len(a["then"]), len(b["then"])))
        return f"decision {i + 1} after the first: {a['then'][i] if i < len(a['then']) else None} -> {b['then'][i] if i < len(b['then']) else None}"
    return None


def decisions(record, ai_roles):
    """[(frame, role, kind, detail)] in the record's order (strict.by_role), but those of its last two frames, which a
    start one frame late can push out of the recording."""
    out = []
    for e in strict.by_role(record["events"]):
        frame, kind, role = e[0], e[2], e[3]
        if frame > record["frame_count"] - 2:
            continue
        if kind == "ability":
            out.append((frame, role, e[4], e[5]))
        elif kind == "bt" and role in ai_roles:
            out.append((frame, role, "bt", e[4]))
        elif kind == "tag" and e[4] == "status.hitstun":
            out.append((frame, role, "stun", e[5]))
        elif kind == "attribute" and e[4] == "Health":
            out.append((frame, role, "health", round(e[6], 4)))
    return out


def compare(reference, run, ai_roles):
    """The first way run differs from reference on these terms, or None."""
    A, B = decisions(reference, ai_roles), decisions(run, ai_roles)
    roles = sorted({d[1] for d in A} | {d[1] for d in B})
    per = lambda D, r: [d for d in D if d[1] == r]
    for r in roles:
        a, b = [d[1:] for d in per(A, r)], [d[1:] for d in per(B, r)]
        if a != b:
            i = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
            return f"{r}'s decision {i}: {a[i] if i < len(a) else None} -> {b[i] if i < len(b) else None}"
    frames = {(r, k): d[0] for r in roles for k, d in enumerate(per(B, r))}
    seen, last_start = {}, {}
    for i, d in enumerate(A):
        k = seen.get(d[1], 0)
        seen[d[1]] = k + 1
        key = (d[1], k)
        fa, fb = d[0], frames[key]
        causes = [(0, 0)]
        if i:
            causes.append(prev)
        if d[2] in ("ended", "cancelled") and (d[1], d[3]) in last_start:
            causes.append(last_start[(d[1], d[3])])
        tol = 1 if d[1] in ai_roles and d[2] in ("activated", "bt") else 0
        if not any(abs((fb - cb) - (fa - ca)) <= tol for ca, cb in causes):
            return f"{d[1]} {d[2]} {str(d[3]).split('/')[-1]}: frame {fa} -> {fb}, from no cause it shares"
        if d[2] == "activated":
            last_start[(d[1], d[3])] = (fa, fb)
        prev = (fa, fb)
    return None


def summary(record, role):
    """What a boss scenario settles: the moves started (frame, ability), the marks it crossed, whether it was stunned."""
    out = {"started": [], "ended": [], "failed": [], "health": [], "stunned": []}
    for e in record["events"]:
        if e[3] != role:
            continue
        if e[2] == "ability" and e[4] == "activated":
            out["started"].append([e[0], e[5].split("/")[-1]])
        elif e[2] == "ability":
            out["ended"].append([e[0], e[5].split("/")[-1], e[4]])
        elif e[2] == "failed":
            out["failed"].append([e[0], e[4].split("/")[-1]])
        elif e[2] == "attribute" and e[4] == "Health":
            out["health"].append([e[0], e[5], e[6]])
        elif e[2] == "tag" and e[4] == "status.hitstun":
            out["stunned"].append([e[0], e[5]])
    return out


def suite(ctx):
    """The boss scenarios and the decision matrix, each compared with the UE4 reference on its terms when there is one."""
    mygame.use(ctx)
    ctx.reset()
    player = tool("pie_player_state")
    ar.PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    base = strict.base_attributes(player)
    ar.DUMMY[0] = {"path": strict.DUMMY_PATH}
    result = {"boss": {}, "matrix": {}}
    for sc in boss_scenarios():
        record = strict.run(sc, base)
        result["boss"][sc["name"]] = record
        print(f"  {sc['name']}: {json.dumps(summary(record, 'boss'))[:250]}")
    for sc in matrix_scenarios():
        result["matrix"][sc["name"]] = matrix_entry(strict.run(sc, base))
    (ctx.out / "ai.json").write_text(json.dumps(result, sort_keys=True))
    (ctx.out / "matrix.json").write_text(json.dumps(result["matrix"], indent=1, sort_keys=True))
    problems = []
    if REFERENCE.exists():
        reference = json.loads(REFERENCE.read_text())
        for name in sorted(set(reference["boss"]) | set(result["boss"])):
            where = compare(reference["boss"][name], result["boss"][name], {"boss"}) if name in reference["boss"] and name in result["boss"] else "missing"
            if where:
                problems.append(f"{name}: {where}")
        for name in sorted(set(reference["matrix"]) | set(result["matrix"])):
            a, b = reference["matrix"].get(name), result["matrix"].get(name)
            where = matrix_difference(a, b) if a and b else "missing"
            if where:
                problems.append(f"{name}: {where}")
    for p in problems:
        print("  " + p)
    return {"ok": not problems, "checks": len(result["boss"]) + len(result["matrix"]), "problems": problems, "files": {}}
