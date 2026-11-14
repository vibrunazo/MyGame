"""Frame rate regression: the moves that hit several times land the same hits at a normal frame step (1x, ~60 fps) and
at a 3x larger one (3x speed). Gameplay that depends on the frame rate would hit differently on a slow PC or in a
hitch.

The `framerate` suite of `unreal-agent test` (`suite(ctx)`). It sets its own speeds (speed "own") and puts the run's
speed back afterwards.
"""
import json

import ability_regression as A
import mygame

# moves that hit several times: frame rate sensitive if anything is
MOVES = ["punch_hold", "kick_hold", "super_punch_runpunch", "super_kick_tat", "super_kick_kkk", "fire_hands_punch"]
SPEEDS = (1.0, 3.0)


def suite(ctx):
    """Each move's damage to the dummy at both speeds: they must match."""
    mygame.use(ctx)
    results = {}
    try:
        for speed in SPEEDS:
            mygame.set_speed(speed)
            ctx.reset()
            results[speed] = A.run_scenarios(MOVES)
    finally:
        mygame.set_speed(ctx.speed)
    (ctx.out / "framerate.json").write_text(json.dumps({str(k): v for k, v in results.items()}, indent=1))
    problems = []
    for name in MOVES:
        slow, fast = results[SPEEDS[0]].get(name), results[SPEEDS[1]].get(name)
        print(f"  {name:24} 1x {slow:>6}   3x {fast:>6}" + ("" if slow == fast else "   <- differs"))
        if slow != fast:
            problems.append(f"{name}: dummy damage {slow} at 1x, {fast} at 3x")
    return {"ok": not problems, "checks": len(MOVES), "problems": problems, "files": {"framerate.json": None}}
