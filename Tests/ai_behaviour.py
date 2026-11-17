"""Step 0c item 4: MyGame's AI behaviour on the current tree (Ass Plan.md, Step 0c and 1.1).

The `ai` suite of `unreal-agent test`. Its scenarios run with the AI on, each seeded (the per-tick reseed of
run_timeline), on fresh spawned enemies, recorded by the strict recorder (the `bt` events give the tree's tasks).

The boss scenarios (exact): the boss's AI on, its movement frozen (Speed 0: its tree still ticks, it can't walk), and
a real hit from the player that crosses its health marks (75, 50 and 25% of MaxHealth):
  boss_idle        the boss hasn't seen the player, who punches its back across 75%
  boss_mid_attack  the boss is punching the player when the player's punch crosses 75%
  boss_two_marks   one punch takes it from 52 to 22 of 100: across 50% and 25%
Each records which of its moves started, on which frame, and whether it was stunned.
"""
import json
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


def summary(record, role):
    """What a boss scenario settles: the moves started (frame, ability), the marks it crossed, whether it was stunned."""
    out = {"started": [], "failed": [], "health": [], "stunned": []}
    for e in record["events"]:
        if e[3] != role:
            continue
        if e[2] == "ability" and e[4] == "activated":
            out["started"].append([e[0], e[5].split("/")[-1]])
        elif e[2] == "failed":
            out["failed"].append([e[0], e[4].split("/")[-1]])
        elif e[2] == "attribute" and e[4] == "Health":
            out["health"].append([e[0], e[5], e[6]])
        elif e[2] == "tag" and e[4] == "status.hitstun":
            out["stunned"].append([e[0], e[5]])
    return out


def suite(ctx):
    mygame.use(ctx)
    ctx.reset()
    player = tool("pie_player_state")
    ar.PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    base = strict.base_attributes(player)
    ar.DUMMY[0] = {"path": strict.DUMMY_PATH}
    results, records = {}, {}
    for sc in boss_scenarios():
        records[sc["name"]] = strict.run(sc, base)
        results[sc["name"]] = summary(records[sc["name"]], "boss")
        print(f"  {sc['name']}: {json.dumps(results[sc['name']])[:300]}")
    (ctx.out / "ai.json").write_text(json.dumps(results, indent=1))
    (ctx.out / "ai_records.json").write_text(json.dumps(records, sort_keys=True))
    problems = []
    if REFERENCE.exists():
        reference = json.loads(REFERENCE.read_text())
        problems = [f"{k}: {json.dumps(reference.get(k))[:200]} -> {json.dumps(results.get(k))[:200]}" for k in sorted(set(reference) | set(results))
                    if reference.get(k) != results.get(k)]
    for p in problems:
        print("  " + p)
    return {"ok": not problems, "checks": len(results), "problems": problems, "files": {}}
