"""Runs the PIE regression suites in one editor session and compares them with Tests/baselines.

    python Tests/run_all.py                      # every suite, game at 3x
    python Tests/run_all.py --speed 1            # real time
    python Tests/run_all.py --only level,camera  # some suites
    python Tests/run_all.py --max-fps 20         # cap the frame rate too
    python Tests/run_all.py --keep-editor        # leave the editor running afterwards

One headless editor, one PIE session: each suite starts a new run with its seed through the game's own StartNewRun
(the main menu's path), instead of restarting the editor. Results go to Tests/out/<time>/; the summary lists each
suite's verdict and how long it took.
"""
import argparse
import json
import sys
import time
from pathlib import Path

import harness

HERE = Path(__file__).resolve().parent
BASE = HERE / "baselines"


def suite_level(out):
    import level_regression as L
    ok = True
    for seed in (6032, 101, 2024):
        harness.start_run(seed)
        result = L.capture_state()
        (out / f"level_seed_{seed}.json").write_text(json.dumps(result, indent=1))
        diffs = L.compare(json.loads((BASE / f"level_seed_{seed}.json").read_text()), result)
        print(f"  seed {seed}: " + ("same level" if not diffs else "differs: " + "; ".join(diffs)))
        ok &= not diffs
    return ok


def suite_abilities(out):
    import ability_regression as A
    harness.start_run(6032)
    result = A.run_all()
    (out / "abilities.json").write_text(json.dumps(result, indent=1))
    lines = A.compare(json.loads((BASE / "abilities_phase3.json").read_text()), result)
    for line in lines:
        print("  " + line)
    return not any(not l.startswith("note") for l in lines)


def suite_loop(out):
    import loop_regression as P
    harness.start_run(6032)
    results = P.run_all()
    (out / "loop.json").write_text(json.dumps(results, indent=1))
    return all(v["pass"] for v in results.values() if isinstance(v, dict) and "pass" in v)


def suite_camera(out):
    import camera_regression as C
    harness.start_run(6032)
    results = C.run_all()
    (out / "camera.json").write_text(json.dumps(results))
    C.report_results(results)
    # the room camera never moves while the player stays put in a room, whatever they do
    still = {k: results[k]["metrics"] for k in ("turns", "lunges", "jumps")}
    return all(m.get("cam_xy") == 0 and m.get("cam_z") == 0 for m in still.values())


# moves that hit several times: frame rate sensitive if anything is
FRAME_SENSITIVE = ["punch_hold", "kick_hold", "super_punch_runpunch", "super_kick_tat", "super_kick_kkk", "fire_hands_punch"]


def suite_framerate(out):
    """The same multi-hit moves at a normal frame step (1x, ~60 fps) and at a 3x larger one (3x speed): the hits
    must match. Gameplay that depends on the frame rate would hit differently on a slow PC or during a hitch."""
    import ability_regression as A
    results = {}
    for speed in (1.0, 3.0):
        harness.set_speed(speed)
        harness.start_run(6032)
        results[speed] = A.run_scenarios(FRAME_SENSITIVE)
    harness.set_speed(ARGS.speed)
    (out / "framerate.json").write_text(json.dumps({str(k): v for k, v in results.items()}, indent=1))
    ok = True
    for name in FRAME_SENSITIVE:
        a, b = results[1.0].get(name), results[3.0].get(name)
        print(f"  {name:24} 1x {a:>6}   3x {b:>6}" + ("" if a == b else "   <- differs"))
        ok &= a == b
    return ok


SUITES = {"level": suite_level, "abilities": suite_abilities, "loop": suite_loop, "camera": suite_camera,
          "framerate": suite_framerate}
ARGS = None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--speed", type=float, default=3.0)
    ap.add_argument("--only", default=",".join(SUITES))
    ap.add_argument("--max-fps", type=float, default=0, help="cap the frame rate (gameplay mustn't depend on it)")
    ap.add_argument("--keep-editor", action="store_true")
    args = ap.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")  # key labels like "Shift + ↑" in the suites' output
    global ARGS
    ARGS = args
    out = HERE / "out" / time.strftime("%Y%m%d-%H%M%S")
    out.mkdir(parents=True)
    t_all = time.time()
    harness.start_editor(log=f"tests-{out.name}")
    print(f"editor up in {time.time() - t_all:.0f} s")
    harness.set_speed(args.speed)
    if args.max_fps:
        harness.tool("set_max_fps", fps=args.max_fps)
    verdicts = {}
    for name in args.only.split(","):
        t0 = time.time()
        print(f"== {name}")
        try:
            verdicts[name] = (SUITES[name](out), time.time() - t0)
        except Exception as e:  # a crashed suite is a failed suite; the others still run
            print(f"  crashed: {e!r}")
            verdicts[name] = (False, time.time() - t0)
    if not args.keep_editor:
        harness.kill_editor()
    print(f"\nspeed {args.speed}x, results in {out}")
    for name, (ok, secs) in verdicts.items():
        print(f"  {'PASS' if ok else 'FAIL'}  {name:10} {secs:6.0f} s")
    print(f"  total {time.time() - t_all:.0f} s")
    sys.exit(0 if all(ok for ok, _ in verdicts.values()) else 1)


if __name__ == "__main__":
    main()
