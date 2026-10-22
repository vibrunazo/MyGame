"""Camera measurements for Phase 6: what the game camera does in scripted situations, as numbers.

Needs the editor running with PIE started and `-Seed=6032` (the start room is open to the right, toward room 0,1).

    python Tests/camera_regression.py run out.json      # record every scenario
    python Tests/camera_regression.py report out.json   # the summary table again

Scenarios (each starts from the same spot, enemies frozen):
  walk_lr      walk right 2 s, stop, walk left 2 s, stop
  walk_depth   walk toward the back wall and back (X, toward and away from the camera)
  turns        stand still, face left and right every 0.6 s, punching (a beat 'em up fight)
  lunges       punch combos facing right, standing still
  jumps        three jumps in place
  dashes       dash right, dash left
  room_change  walk right into the next room

Per scenario: how far the camera travelled (xy and z separately), how far the player did, the camera's travel while
the player stood still, its yaw/pitch/FoV range, "busy-ness" (camera direction reversals per second), and the sideways
look-ahead (camera Y minus player Y: positive looks right) at the end and its range.
"""
import json
import math
import sys
import time

from harness import wait_game
from harness import tool
from level_regression import wait_loaded

START = (0.0, -300.0)


def freeze_enemies():
    for c in tool("list_characters"):
        if c["team"] == 1:
            tool("set_ai_enabled", actor_path=c["path"], enabled=False, show_player=False)


def reset():
    tool("move_input", forward=0.0, right=0.0)
    z = tool("floor_height", x=START[0], y=START[1], from_z=500.0)["z"] + 100
    tool("teleport_player", x=START[0], y=START[1], z=z)
    tool("set_player_facing", yaw=90.0)
    wait_game(2.5)  # let the camera settle


def move(f, r, secs):
    tool("move_input", forward=f, right=r)
    wait_game(secs)
    tool("move_input", forward=0.0, right=0.0)


def punch():
    tool("press_input", button="Punch", pressed=True)
    wait_game(0.08)
    tool("press_input", button="Punch", pressed=False)


SCENARIOS = {
    "walk_lr": lambda: (move(0, 1, 2.0), wait_game(1.0), move(0, -1, 2.0), wait_game(1.5)),
    "walk_depth": lambda: (move(1, 0, 1.2), wait_game(1.0), move(-1, 0, 1.2), wait_game(1.5)),
    "turns": lambda: ([(tool("set_player_facing", yaw=90.0 if i % 2 else -90.0), punch(), wait_game(0.5)) for i in range(10)],
                      wait_game(1.5)),
    "lunges": lambda: ([(punch(), wait_game(0.35)) for _ in range(8)], wait_game(1.5)),
    "jumps": lambda: ([(tool("jump_player"), wait_game(1.1)) for _ in range(3)], wait_game(1.0)),
    "dashes": lambda: (tool("dash_player", forward=0.0, right=1.0), wait_game(1.2), tool("dash_player", forward=0.0, right=-1.0), wait_game(1.5)),
    "tile_change": lambda: (move(0, 1, 4.5), wait_game(2.0)),
}


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


def run(out):
    results = run_all()
    with open(out, "w") as f:
        json.dump(results, f)
    report(out)


def run_all():
    wait_loaded()
    freeze_enemies()
    tool("set_player_attribute", attribute="Mana", value=1000.0)
    results = {}
    for name, act in SCENARIOS.items():
        reset()
        tool("camera_record", action="start")
        act()
        rec = tool("camera_record", action="stop")
        results[name] = {"metrics": metrics(rec["samples"]), "samples": rec["samples"]}
        print(name, results[name]["metrics"])
    return results


def report(path):
    report_results(json.load(open(path)))


def report_results(r):
    cols = ["lead_y_end", "lead_y_min", "lead_y_max", "cam_xy", "cam_z", "player_xy", "cam_per_player", "cam_while_still", "yaw_range", "pitch_range", "fov_range", "reversals_per_s"]
    print(f"{'scenario':12}" + "".join(f"{c:>17}" for c in cols))
    for name, v in r.items():
        m = v["metrics"]
        print(f"{name:12}" + "".join(f"{str(m.get(c)):>17}" for c in cols))


if __name__ == "__main__":
    (run if sys.argv[1] == "run" else report)(sys.argv[2])
