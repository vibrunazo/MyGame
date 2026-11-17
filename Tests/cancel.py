"""Step 0c item 3: the cancel matrix (Ass Plan.md, Step 0c and 3.5).

The `cancel` suite of `unreal-agent test`. For every ability A of the player, each enemy and the boss, one run with a
target and one without: A starts, and every frame the strict recorder asks the engine which of the character's granted
abilities could start now (CanActivateAbility on each, as a press asks it, starting nothing; A itself included, so an
ability pressed while it runs is covered). Each frame is labelled with the state that matters:

  running / after   A is active, or has ended
  hit               Ass.Combo.CanCancel is on (a hit confirmed)
  window / ended    A's hit window is open, or has closed (no label before it opens)
  airborne          the character is falling

and the matrix keeps, per run, each state in the order it came with the abilities that could start in it (a state
whose set changes inside it appears again). About 40 runs; the frames themselves stay in the output.

The matrix must equal the committed baseline (BASELINE, recorded on the UE5 mannequin after the move, Step 0c), or
before it existed the UE4 reference (REFERENCE, kept outside git).
"""
import json
from pathlib import Path

import mygame
from mygame import tool
import ability_regression as ar
import strict
from strict import scenario, enemy, tap, press, learn, at, activate, target_player, X, Y, CLOSE

ROOT = Path(__file__).resolve().parent.parent
REFERENCE = ROOT / "Saved" / "AssReference" / "cancel.json"
BASELINE = ROOT / "Tests" / "baselines" / "cancel.json"  # recorded on the UE5 mannequin after the move (Step 0c)
FAR = X - 700  # an enemy's target out of reach: its attack misses


def player_runs():
    jump = lambda button: tap("Jump", 0.0, 0.2) + tap(button, 0.25)
    supers = {"smash": ("DA_LearnSmash", "Punch"), "runpunch": ("DA_LearnRunPunch", "Punch"), "uppercut": ("DA_LearnUppercut", "Punch"),
              "tat": ("DA_LearnTat", "Kick"), "kkk": ("DA_LearnKkk", "Kick")}
    moves = {"punch": (tap("Punch", 0.0), 2.0), "kick": (tap("Kick", 0.0), 2.0), "air_punch": (jump("Punch"), 2.2),
             "air_kick": (jump("Kick"), 2.2), "dash": ([at(0.0, "dash_player")], 1.5),
             "cast": (learn(0.0, "DA_LearnFireball") + tap("Cast", 0.2), 2.5)}
    for name, (item, button) in supers.items():
        moves[f"super_{name}"] = (learn(0.0, item) + press("Super", 0.1) + tap(button, 0.2) + press("Super", 0.5, False), 2.8)
    for name, (inputs, duration) in moves.items():
        yield scenario(f"{name}_hit", inputs, duration), "player"
        yield scenario(f"{name}_miss", inputs, duration, enemies=[]), "player"


def enemy_runs():
    for kind in ("enemy1", "enemy2", "enemy3", "kicker", "wizard"):
        x = X + 300 if kind == "wizard" else X
        yield scenario(f"{kind}_hit", activate(0.0, "enemy", kind), 3.0, enemies=[enemy("enemy", kind, x=x)],
                       player={"Defense": 1.0}, setup=[target_player("enemy")]), "enemy"
        yield scenario(f"{kind}_miss", activate(0.0, "enemy", kind), 3.0, enemies=[enemy("enemy", kind, x=x)],
                       player_x=FAR, player={"Defense": 1.0}, setup=[target_player("enemy")]), "enemy"
    for ability, duration in (("boss_punches", 4.0), ("boss_smash", 8.0), ("boss_summon", 1.5), ("boss_debuff", 1.5)):
        for name, px in (("hit", CLOSE), ("miss", FAR)):
            yield scenario(f"{ability}_{name}", activate(0.0, "boss", ability), duration, enemies=[enemy("boss", "boss", x=X + 30)],
                           player_x=px, player={"Defense": 1.0}, setup=[target_player("boss")]), "boss"


def runs():
    ar.DUMMY[0] = {"path": strict.DUMMY_PATH}
    yield from player_runs()
    yield from enemy_runs()


def states(record, role):
    """[[label, [abilities that could start]], ...]: each state in the order it came, consecutive repeats folded."""
    events = sorted(record["events"], key=lambda e: e[0])
    out, i, first, active, ended, cancancel, window = [], 0, None, False, False, 0, None
    for row in record["frames"][role]:
        frame = row[0]
        while i < len(events) and events[i][0] <= frame:
            e = events[i]
            i += 1
            if e[3] != role:
                continue
            if e[2] == "ability" and e[4] == "activated" and first is None:
                first, active = e[5], True
            elif e[2] == "ability" and e[4] in ("ended", "cancelled") and e[5] == first and active:
                active, ended = False, True
            elif e[2] == "tag" and e[4] == "Ass.Combo.CanCancel":
                cancancel = e[5]
            elif e[2] == "anim" and "Hitbox" in e[6] and e[4] in ("begin", "end"):
                window = "window" if e[4] == "begin" else "ended"
        if first is None:
            continue
        label = "+".join(x for x in ("running" if active else "after", "hit" if cancancel else "", window or "",
                                     "airborne" if row[12] == 3 else "") if x)
        can = row[13]
        if not out or out[-1] != [label, can]:
            out.append([label, can])
    return {"ability": first, "states": out}


def suite(ctx):
    mygame.use(ctx)
    ctx.reset()
    player = tool("pie_player_state")
    ar.PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    base = strict.base_attributes(player)
    matrix, records = {}, {}
    for sc, role in runs():
        records[sc["name"]] = strict.run(sc, base, query=[role])
        matrix[sc["name"]] = states(records[sc["name"]], role)
        print(f"  {sc['name']}: {matrix[sc['name']]['ability']}, {len(matrix[sc['name']]['states'])} states")
    (ctx.out / "cancel.json").write_text(json.dumps(matrix, indent=1, sort_keys=True))
    (ctx.out / "cancel_records.json").write_text(json.dumps(records, sort_keys=True))
    problems = [f"{name}: no ability started" for name, m in matrix.items() if not m["ability"]]
    against = BASELINE if BASELINE.exists() else REFERENCE
    if against.exists():
        reference = json.loads(against.read_text())
        for name in sorted(set(reference) | set(matrix)):
            if reference.get(name) != matrix.get(name):
                problems.append(f"{name}: {json.dumps(reference.get(name))[:300]} -> {json.dumps(matrix.get(name))[:300]}")
    for p in problems:
        print("  " + p)
    return {"ok": not problems, "checks": len(matrix), "problems": problems, "files": {"cancel.json": "Tests/baselines/cancel.json"}}
