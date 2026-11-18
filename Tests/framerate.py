"""Frame rate regression: gameplay must not depend on the frame step (Ass Plan.md Step 8's gate).

The `framerate` suite of `unreal-agent test` (`suite(ctx)`). It sets its own speeds and steps (speed "own") and puts
the run's back afterwards.

- The moves that hit several times land the same hits at a normal frame step (1x, 1/60 s) and at a 3x larger one
  (3x speed): a slow PC or a hitch must not change them.
- Every scenario of the abilities suite has the same whole outcome (`ability_regression.outcome`: montages, abilities,
  hit, launched, pushed, mana, jumped, moved, tags, damage, lingering) at 1/60 s, at 1/30 s, and on a seeded uneven step
  (1/60 s with a 1/20 s frame every 7, seed 7). Tolerances: none beyond the outcome's own (its distances are thresholds,
  its damage exact except the multi-hit holds, its tags exact, a brief tag missing between samples only a note).
"""
import json

from unreal_agent_tools.testing import PLAY

import ability_regression as A
import mygame

# moves that hit several times: frame rate sensitive if anything is
MOVES = ["punch_hold", "kick_hold", "super_punch_runpunch", "super_kick_tat", "super_kick_kkk", "fire_hands_punch"]
SPEEDS = (1.0, 3.0)
# (name, PushFixedStep arguments)
STEPS = (("1/60", {"fps": 60}), ("1/30", {"fps": 30}),
         ("uneven", {"fps": 60, "longFrameEvery": 7, "longFrameSeconds": 0.05, "seed": 7}))


def all_scenarios():
    """Every abilities scenario's full record, in order."""
    player = mygame.tool("pie_player_state")
    A.PLAYER_PATH[0] = next(c["path"] for c in mygame.tool("list_characters") if c["actor"] == player["actor"])
    dummy = A.pick_dummy()
    A.DUMMY[0] = dummy
    return {"scenarios": [A.run_scenario(name, inputs, duration, A.PLAYER_PATH[0], dummy, bool(start_room))
                          for name, inputs, duration, *start_room in A.scenarios()]}


def suite(ctx):
    mygame.use(ctx)
    results, outcomes = {}, {}
    try:
        for speed in SPEEDS:
            mygame.set_speed(speed)
            ctx.reset()
            results[speed] = A.run_scenarios(MOVES)
    finally:
        mygame.set_speed(ctx.speed)
    for name, step in STEPS:
        ctx.ue.call_tool(PLAY, "PushFixedStep", step)
        try:
            ctx.reset()
            outcomes[name] = all_scenarios()
        finally:
            ctx.ue.call_tool(PLAY, "PopFixedStep", {})
    (ctx.out / "framerate.json").write_text(json.dumps({**{str(k): v for k, v in results.items()}, **outcomes}, indent=1))
    problems = []
    for name in MOVES:
        slow, fast = results[SPEEDS[0]].get(name), results[SPEEDS[1]].get(name)
        print(f"  {name:24} 1x {slow:>6}   3x {fast:>6}" + ("" if slow == fast else "   <- differs"))
        if slow != fast:
            problems.append(f"{name}: dummy damage {slow} at 1x, {fast} at 3x")
    reference = outcomes[STEPS[0][0]]
    for name, _ in STEPS[1:]:
        lines = A.compare(reference, outcomes[name])
        for line in lines:
            print(f"  {name}: {line}")
        problems += [f"{name}: {line}" for line in lines if not line.startswith("note")]
    checks = len(MOVES) + len(reference["scenarios"]) * (len(STEPS) - 1)
    return {"ok": not problems, "checks": checks, "problems": problems, "files": {"framerate.json": None}}
