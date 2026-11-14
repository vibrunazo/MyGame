"""Camera measurements for Phase 6: what the game camera does in scripted situations, as numbers.

The `camera` suite of `unreal-agent test` (`suite(ctx)`), on seed 6032 (the start room is open to the right, toward
room 0,1). Each scenario plays inside the engine on the game clock (the run_timeline hook), with the camera recorder
started and stopped inside it, so on the fixed step two runs give the same numbers to the last digit. It asserts that
the camera stays still while the player stays in place, compares every scenario's numbers with
Tests/baselines/camera_room.json exactly, and checks each room's wall cutaway.

    python Tests/camera_regression.py report out.json   # the summary table of a result file again

Scenarios (each starts from the same spot, enemies frozen):
  walk_lr      walk right 2 s, stop, walk left 2 s, stop
  walk_depth   walk toward the back wall and back (X, toward and away from the camera)
  turns        stand still, face left and right every 0.58 s, punching (a beat 'em up fight)
  lunges       punch combos facing right, standing still
  jumps        three jumps in place
  dashes       dash right, dash left
  tile_change  walk right into the next tile

Per scenario: how far the camera travelled (xy and z separately), how far the player did, the camera's travel while
the player stood still, its yaw/pitch/FoV range, "busy-ness" (camera direction reversals per second), and the sideways
look-ahead (camera Y minus player Y: positive looks right) at the end and its range.
"""
import json
import math
import sys
import time
from pathlib import Path

import mygame
from mygame import floor_z, tool, wait_game, wait_loaded

START = (0.0, -300.0)
BASELINE = Path(__file__).resolve().parent / "baselines" / "camera_room.json"


def freeze_enemies():
    for c in tool("list_characters"):
        if c["team"] == 1:
            tool("set_ai_enabled", actor_path=c["path"], enabled=False, show_player=False)


def reset():
    tool("move_input", forward=0.0, right=0.0)
    tool("teleport_player", x=START[0], y=START[1], z=floor_z(*START) + 100)
    tool("set_player_facing", yaw=90.0)
    wait_game(2.5)  # the camera settles and snaps to rest


# timeline inputs: [game time, "call", MyGameTools hook, kwargs], at most one per frame
def at(t, hook, **kwargs):
    return [round(t, 4), "call", hook, kwargs]


def move(t, forward, right, secs):
    return [at(t, "move_input", forward=float(forward), right=float(right)), at(t + secs, "move_input", forward=0.0, right=0.0)]


def punch(t):
    return [at(t, "press_input", button="Punch", pressed=True), at(t + 0.08, "press_input", button="Punch", pressed=False)]


# each scenario: its inputs on the game clock, and when it ends
SCENARIOS = {
    "walk_lr": (move(0.0, 0, 1, 2.0) + move(3.0, 0, -1, 2.0), 6.5),
    "walk_depth": (move(0.0, 1, 0, 1.2) + move(2.2, -1, 0, 1.2), 4.9),
    "turns": ([i for k in range(10) for i in [at(0.58 * k, "set_player_facing", yaw=90.0 if k % 2 else -90.0)] + punch(0.58 * k)], 7.3),
    "lunges": ([i for k in range(8) for i in punch(0.43 * k)], 4.94),
    "jumps": ([at(1.1 * k, "jump_player") for k in range(3)], 4.3),
    "dashes": ([at(0.0, "dash_player", forward=0.0, right=1.0), at(1.2, "dash_player", forward=0.0, right=-1.0)], 2.7),
    "tile_change": (move(0.0, 0, 1, 4.5), 6.5),
}


def play(inputs, end):
    """One scenario inside the engine: the recorder starts at 0 and stops at `end`, both on the game clock."""
    spec = {"dummy": None, "duration": end + 0.05, "settle": 0.0,
            "inputs": [at(0.0, "camera_record", action="start")] + inputs + [at(end, "camera_record", action="stop")]}
    tool("run_timeline", spec=json.dumps(spec))
    while not tool("timeline_result")["done"]:
        time.sleep(0.05)
    return tool("camera_record", action="stop")["samples"]  # already stopped inside: the same samples


def metrics(samples):
    s = samples
    if len(s) < 3:
        return {"frames": len(s)}
    d = lambda a, b, i, j: math.dist(a[i:j], b[i:j])
    cam_xy = sum(d(s[k], s[k + 1], 1, 3) for k in range(len(s) - 1))
    cam_z = sum(abs(s[k + 1][3] - s[k][3]) for k in range(len(s) - 1))
    ply = sum(d(s[k], s[k + 1], 7, 9) for k in range(len(s) - 1))
    still = 0.0
    reversals = 0
    for k in range(1, len(s) - 1):
        dt = max(s[k + 1][0] - s[k][0], 1e-4)
        player_speed = d(s[k], s[k + 1], 7, 9) / dt
        cam_step = d(s[k], s[k + 1], 1, 4)
        if player_speed < 20.0:
            still += cam_step
        v0 = (s[k][1] - s[k - 1][1], s[k][2] - s[k - 1][2])
        v1 = (s[k + 1][1] - s[k][1], s[k + 1][2] - s[k][2])
        if math.hypot(*v0) > 0.5 and math.hypot(*v1) > 0.5 and v0[0] * v1[0] + v0[1] * v1[1] < 0:
            reversals += 1
    dur = s[-1][0] - s[0][0]
    rng = lambda i: round(max(x[i] for x in s) - min(x[i] for x in s), 2)
    # how far the camera looks ahead sideways: camera Y minus player Y (the arm only tilts along X)
    lead = [x[2] - x[8] for x in s]
    return {"lead_y_end": round(lead[-1]), "lead_y_min": round(min(lead)), "lead_y_max": round(max(lead)),
            "frames": len(s), "seconds": round(dur, 2), "cam_xy": round(cam_xy), "cam_z": round(cam_z), "player_xy": round(ply),
            "cam_per_player": round(cam_xy / ply, 2) if ply > 1 else None, "cam_while_still": round(still),
            "yaw_range": rng(5), "pitch_range": rng(4), "fov_range": rng(6), "reversals_per_s": round(reversals / max(dur, 1e-3), 2)}


def suite(ctx):
    """The room camera never moves while the player stays put in a room, whatever they do; every scenario's numbers
    are the baseline's; in every room the walls between it and the camera are cut away, and only those."""
    mygame.use(ctx)
    ctx.reset()
    results = run_all()
    (ctx.out / "camera.json").write_text(json.dumps(results))
    numbers = {name: r["metrics"] for name, r in results.items()}
    (ctx.out / BASELINE.name).write_text(json.dumps(numbers, indent=1))
    report_results(results)
    problems = []
    for name in ("turns", "lunges", "jumps"):
        m = results[name]["metrics"]
        if m.get("cam_xy") != 0 or m.get("cam_z") != 0:
            problems.append(f"{name}: the camera moved {m.get('cam_xy')} across and {m.get('cam_z')} up, the player in place")
    expected = json.loads(BASELINE.read_text())
    for name, m in expected.items():
        got = numbers.get(name, {})
        problems += [f"{name}.{k}: {v} -> {got.get(k)}" for k, v in m.items() if got.get(k) != v]
    cut = cutaway_check()
    (ctx.out / "cutaway.json").write_text(json.dumps(cut, indent=1))
    for room, r in cut.items():
        print(f"  cutaway {room}: " + ("ok" if r["pass"] else f"lowered {r['lowered']}, expected {r['expected']}"))
        if not r["pass"]:
            problems.append(f"cutaway {room}: lowered {r['lowered']}, expected {r['expected']}")
    return {"ok": not problems, "checks": 3 + len(expected) + len(cut), "problems": problems,
            "files": {"camera.json": None, BASELINE.name: "Tests/baselines/" + BASELINE.name, "cutaway.json": None}}


def run_all():
    wait_loaded()
    freeze_enemies()
    tool("set_player_attribute", attribute="Mana", value=1000.0)
    results = {}
    for name, (inputs, end) in SCENARIOS.items():
        reset()
        samples = play(inputs, end)
        results[name] = {"metrics": metrics(samples), "samples": samples}
        print(name, results[name]["metrics"])
    return results


def cutaway_check():
    """Walks into every room (a teleport to its first tile's center): the walls lowered must be exactly that room's
    walls on the camera side, each of its tiles' bottom wall (a tile joined to the one below has none)."""
    freeze_enemies()
    s = tool("level_state")
    sx, sy = s["tile_size"]
    walls = {tuple(w["at"]) for w in s["walls"]}
    out = {}
    for room in s["rooms"]:
        x, y = room[0][0] * sx, room[0][1] * sy
        tool("teleport_player", x=x, y=y, z=floor_z(x, y) + 100)
        wait_game(1.0)
        now = tool("level_state")["walls"]
        lowered = sorted(tuple(w["at"]) for w in now if w["height"] < 100)
        expected = sorted(at for at in ((round((t[0] - 0.5) * sx), round(t[1] * sy)) for t in room) if at in walls)
        out[str(room)] = {"pass": lowered == expected, "lowered": lowered, "expected": expected}
    reset()
    return out


def report(path):
    report_results(json.load(open(path)))


def report_results(r):
    cols = ["lead_y_end", "lead_y_min", "lead_y_max", "cam_xy", "cam_z", "player_xy", "cam_per_player", "cam_while_still", "yaw_range", "pitch_range", "fov_range", "reversals_per_s"]
    print(f"{'scenario':12}" + "".join(f"{c:>17}" for c in cols))
    for name, v in r.items():
        m = v["metrics"]
        print(f"{name:12}" + "".join(f"{str(m.get(c)):>17}" for c in cols))


if __name__ == "__main__":
    report(sys.argv[2])
