"""Step 0c item 4: MyGame's AI behaviour on the current tree (Ass Plan.md, Step 0c and 1.1).

The `ai` suite of `unreal-agent test`. Its scenarios run with the AI on, each seeded (the per-tick reseed of
run_timeline), on fresh spawned enemies, recorded by the strict recorder (the `bt` events give the tree's tasks).

The boss scenarios (exact): the boss's AI on, its movement frozen (Speed 0: its tree still ticks, it can't walk), and
a real hit from the player that crosses its health marks (75, 50 and 25% of MaxHealth):
  boss_idle        the boss hasn't seen the player, who punches its back across 75%
  boss_mid_attack  the boss is punching the player when the player's punch crosses 75%
  boss_two_marks   one punch takes it from 52 to 22 of 100: across 50% and 25%
Each records which of its moves started, on which frame, and whether it was stunned.

The token tie (exact): two enemies qualify for the player's attack token on the same tick (both woken by one call, at
the same distance on either side of the player's back), and who takes it is recorded. Each spot is run under both role
names, since the recorder makes the roles tick in their names' order: if the winner follows the name, the tie is
broken by tick order and not by anything in the game.

The stunned holder (exact): enemy a holds the player's token (put on its aggro list), b is beside it and qualifies,
both woken by one call with the player's back to them; at 0.5 s a is stunned for 1.5 s. Recorded: the token's holders
as they change (the distinct enemies on the list), each with its frame.

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
# The UE4 reference names the abilities by their old classes; since Ass Plan.md Step 4 they are templates (a UE4
# recording can't be re-recorded, so the names are mapped when it is read)
REFERENCE_NAMES = {
    "/Game/Abilities/GA_BasicPunches": "/Game/Abilities/AT_Abilities_BasicPunches",
    "/Game/Blueprints/Chars/Boss1/GA_BossPunches": "/Game/Blueprints/Chars/Boss1/AT_Boss1_BossPunches",
    "/Game/Blueprints/Chars/Boss1/GA_BossSelfDebuff": "/Game/Blueprints/Chars/Boss1/AT_Boss1_BossSelfDebuff",
    "/Game/Blueprints/Chars/Boss1/GA_BossSmash1": "/Game/Blueprints/Chars/Boss1/AT_Boss1_BossSmash1",
    "/Game/Blueprints/Chars/Boss1/GA_BossSummon1": "/Game/Blueprints/Chars/Boss1/AT_Boss1_BossSummon1",
    "/Game/Blueprints/Chars/EKicker/GA_KickerKick": "/Game/Blueprints/Chars/EKicker/AT_EKicker_KickerKick",
    "/Game/Blueprints/Chars/EMelee1/GA_DamageEnemy1": "/Game/Blueprints/Chars/EMelee1/AT_EMelee1_DamageEnemy1",
    "/Game/Blueprints/Chars/EMelee2/GA_DamageEnemy1": "/Game/Blueprints/Chars/EMelee2/AT_EMelee2_DamageEnemy1",
    "/Game/Blueprints/Chars/EMelee3/GA_DamageEnemy2": "/Game/Blueprints/Chars/EMelee3/AT_EMelee3_DamageEnemy2",
    "/Game/Blueprints/Chars/EWizard/GA_Cast1": "/Game/Blueprints/Chars/EWizard/AT_EWizard_Cast1",
}


def read_reference():
    """The UE4 reference, its ability names as today's templates."""
    text = REFERENCE.read_text()
    for old, new in REFERENCE_NAMES.items():
        text = text.replace(f'"{old}"', f'"{new}"').replace(f'"{old}.', f'"{new}.')
    return json.loads(text)
# the stun tag, and its name before Ass Plan.md Step 2 renamed it (the UE4 reference outside git holds that one)
STUN_TAGS = ("Ass.Status.HitStun", "status.hitstun")
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
HITSTUN = "/Script/AssCombat.AssHitStunEffect"  # the plugin's stun (Ass Plan.md Step 4, item 5)


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
                                             set_by_caller=json.dumps({"Ass.Data.HitStun": 0.6})))
                        inputs.append(at(0.0, "set_ai_enabled", actor_path="@enemy", enabled=True))
                        yield scenario(f"{kind}.{facing}.{rng}.{token}.{control}", inputs, 1.0, enemies=enemies,
                                       player_x=X - gap, player_yaw=yaw, setup=setup, wait=0.5)


def main_task(snapshot):
    """The main tree's (BT_AIBase's) task in a snapshot of the active tasks, or None."""
    found = re.findall(r"BT_AIBase::([^\]]+\])", snapshot)
    return found[-1] if found else None


def matrix_entry(record, role="enemy"):
    """{"first": [kind, detail, frame], "then": [[kind, detail], ...]} of the enemy's decisions in the second: its
    abilities started, ended or cancelled, its stuns, and the main tree's tasks it moves to (decisions())."""
    ds = [d for d in decisions(record, {role}) if d[1] == role and d[2] in ("activated", "ended", "cancelled", "bt", "stun")]
    first = next((i for i, d in enumerate(ds) if d[2] in ("activated", "bt")), None)
    if first is None:
        return {"first": None, "then": []}
    return {"first": [ds[first][2], ds[first][3], ds[first][0]], "then": [[d[2], d[3]] for d in ds[first + 1:]]}


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


def tie_scenarios():
    """The token tie at two spots, each under both role names (a ticks before b)."""
    left, right = (CLOSE + 70, Y - 60), (CLOSE + 70, Y + 60)
    for spots, name in (((left, right), "a_left"), ((right, left), "a_right")):
        enemies = [enemy("a", "enemy1", x=spots[0][0], y=spots[0][1], yaw=180.0), enemy("b", "enemy1", x=spots[1][0], y=spots[1][1], yaw=180.0)]
        yield scenario(f"token_tie_{name}", [at(0.0, "wake_ai", actor_paths=json.dumps(["@a", "@b"]))], 1.0, enemies=enemies,
                       player_yaw=180.0, player={"Defense": 1000.0}, wait=0.5)


def holder_scenarios():
    enemies = [enemy("a", "enemy1", x=CLOSE + 70, y=Y - 60, yaw=180.0), enemy("b", "enemy1", x=CLOSE + 70, y=Y + 60, yaw=180.0)]
    stun = at(0.5, "apply_effect", actor_path="@a", effect_class_path=HITSTUN, set_by_caller=json.dumps({"Ass.Data.HitStun": 1.5}))
    yield scenario("token_stunned_holder", [at(0.0, "wake_ai", actor_paths=json.dumps(["@a", "@b"])), stun], 3.5, enemies=enemies, player_yaw=180.0,
                   player={"Defense": 1000.0}, wait=0.5, setup=[["aggro_add", {"actor_path": "@player", "target_path": "@a"}]])


def holders(record):
    """[[frame, [the distinct enemies on the player's aggro list]], ...] as they change."""
    out = []
    for e in record["events"]:
        if e[2] == "token" and e[3] == "player":
            now = sorted(set(e[4]))
            if not out or out[-1][1] != now:
                out.append([e[0], now])
    return out


def holders_difference(a, b):
    if [x[1] for x in a] != [x[1] for x in b]:
        return f"holders {a} -> {b}"
    if any(abs(x[0] - y[0]) > 1 for x, y in zip(a, b)):
        return f"frames {a} -> {b}"
    return None


def first_holder(record):
    """Who first holds the player's token, and on which frame."""
    return next(([e[0], e[4]] for e in record["events"] if e[2] == "token" and e[3] == "player" and e[4]), None)


def decisions(record, ai_roles):
    """[(frame, role, kind, detail)] in the record's order (strict.by_role), but those of its last two frames, which a
    start one frame late can push out of the recording; a bt decision is the main tree's task it moves to."""
    out, task = [], {}
    for e in strict.by_role(record["events"]):
        frame, kind, role = e[0], e[2], e[3]
        if frame > record["frame_count"] - 2:
            continue
        if kind == "ability":
            out.append((frame, role, e[4], e[5]))
        elif kind == "bt" and role in ai_roles:
            # the main tree's task the snapshot shows, repeats folded: a task shorter than a frame shows in one recording
            # and not another (the attack subtree's tasks are left to the abilities they start)
            now = main_task(e[4])
            if now and now != task.get(role):
                out.append((frame, role, "bt", now))
            task[role] = now or task.get(role)
        elif kind == "tag" and e[4] in STUN_TAGS:
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
        elif e[2] == "tag" and e[4] in STUN_TAGS:
            out["stunned"].append([e[0], e[5]])
    return out


def suite(ctx):
    """The boss scenarios and the decision matrix, each compared with the UE4 reference on its terms when there is one."""
    return _run(ctx, lambda kind: True, boss=True)


# Ass Step 7: the enemy types on patterns so far, by their controller class (every base enemy runs BP_AIController)
PATTERNS = {"/Game/Blueprints/AI/BP_AIController.BP_AIController_C": "/Game/Blueprints/AI/Patterns/PA_Melee"}
PATTERN_KINDS = ("enemy1", "enemy2", "enemy3")


def pattern_suite(ctx):
    """Step 7: the enemy types on patterns so far (PATTERNS), compared with the UE4 reference on the same terms as the
    tree: their matrix rows and the token scenarios (whose enemies are base enemies)."""
    mygame.use(ctx)
    ctx.reset()
    tool("use_patterns", patterns=json.dumps(PATTERNS))
    try:
        return _run(ctx, lambda kind: kind in PATTERN_KINDS, boss=False, reset=False)
    finally:
        tool("use_patterns", patterns=json.dumps({cls: "" for cls in PATTERNS}))


def _run(ctx, kinds, boss, reset=True):
    """The scenarios (the boss's if boss, the token ones, the matrix rows of the kinds kinds() takes), compared with the
    UE4 reference on its terms when there is one."""
    mygame.use(ctx)
    if reset:
        ctx.reset()
    player = tool("pie_player_state")
    ar.PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    base = strict.base_attributes(player)
    ar.DUMMY[0] = {"path": strict.DUMMY_PATH}
    result = {"boss": {}, "matrix": {}, "tie": {}, "holder": {}}
    for sc in boss_scenarios() if boss else ():
        record = strict.run(sc, base)
        result["boss"][sc["name"]] = record
        print(f"  {sc['name']}: {json.dumps(summary(record, 'boss'))[:250]}")
    for sc in tie_scenarios():
        result["tie"][sc["name"]] = first_holder(strict.run(sc, base))
        print(f"  {sc['name']}: {result['tie'][sc['name']]}")
    for sc in holder_scenarios():
        result["holder"][sc["name"]] = holders(strict.run(sc, base))
        print(f"  {sc['name']}: {result['holder'][sc['name']]}")
    for sc in matrix_scenarios():
        if kinds(sc["name"].split(".")[0]):
            result["matrix"][sc["name"]] = matrix_entry(strict.run(sc, base))
    (ctx.out / "ai.json").write_text(json.dumps(result, sort_keys=True))
    (ctx.out / "matrix.json").write_text(json.dumps(result["matrix"], indent=1, sort_keys=True))
    problems = []
    if REFERENCE.exists():
        reference = read_reference()
        if not boss:
            reference["boss"] = {}
        reference["matrix"] = {k: v for k, v in reference["matrix"].items() if kinds(k.split(".")[0])}
        for name in sorted(set(reference["boss"]) | set(result["boss"])):
            where = compare(reference["boss"][name], result["boss"][name], {"boss"}) if name in reference["boss"] and name in result["boss"] else "missing"
            if where:
                problems.append(f"{name}: {where}")
        for name in sorted(set(reference.get("holder", {})) | set(result["holder"])):
            a, b = reference.get("holder", {}).get(name), result["holder"].get(name)
            where = holders_difference(a, b) if a is not None and b is not None else "missing"
            if where:
                problems.append(f"{name}: {where}")
        for name in sorted(set(reference.get("tie", {})) | set(result["tie"])):
            if reference.get("tie", {}).get(name) != result["tie"].get(name):
                problems.append(f"{name}: {reference.get('tie', {}).get(name)} -> {result['tie'].get(name)}")
        for name in sorted(set(reference["matrix"]) | set(result["matrix"])):
            a, b = reference["matrix"].get(name), result["matrix"].get(name)
            where = matrix_difference(a, b) if a and b else "missing"
            if where:
                problems.append(f"{name}: {where}")
    for p in problems:
        print("  " + p)
    return {"ok": not problems, "checks": len(result["boss"]) + len(result["matrix"]), "problems": problems, "files": {}}
