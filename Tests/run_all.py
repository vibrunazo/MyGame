"""Runs the PIE regression suites in one editor session.

    python Tests/run_all.py                      # every suite, game at 3x
    python Tests/run_all.py --speed 1            # real time
    python Tests/run_all.py --only level,camera  # some suites
    python Tests/run_all.py --max-fps 20         # cap the frame rate too
    python Tests/run_all.py --keep-editor        # leave the editor running afterwards

Each suite follows UnrealAgentTools' pie-suite contract (`unreal-agent guide suites`): `suite(ctx)` in its own file,
returning {ok, checks, problems, files}, so the shared runner can call it unchanged when it comes. Until then this
script is the runner: one headless editor, one PIE session, a ctx whose reset() starts a new run on seed 6032 through
the game's own StartNewRun (the main menu's path). Results go to Tests/out/<time>/ with a summary.json.
"""
import argparse
import importlib
import json
import sys
import time
from pathlib import Path
from types import SimpleNamespace

import harness

HERE = Path(__file__).resolve().parent
# suite name: the file whose suite(ctx) runs it
SUITES = {"level": "level_regression", "abilities": "ability_regression", "loop": "loop_regression",
          "camera": "camera_regression", "framerate": "framerate"}
RESET_SEED = 6032  # the known state: a new run on this seed, loaded and unfrozen


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--speed", type=float, default=3.0)
    ap.add_argument("--only", default=",".join(SUITES))
    ap.add_argument("--max-fps", type=float, default=0, help="cap the frame rate (gameplay mustn't depend on it)")
    ap.add_argument("--keep-editor", action="store_true")
    args = ap.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")  # key labels like "Shift + ↑" in the suites' output
    out = HERE / "out" / time.strftime("%Y%m%d-%H%M%S")
    out.mkdir(parents=True)
    t_all = time.time()
    harness.start_editor(log=f"tests-{out.name}")
    print(f"editor up in {time.time() - t_all:.0f} s")
    harness.set_speed(args.speed)
    if args.max_fps:
        harness.tool("set_max_fps", fps=args.max_fps)
    ctx = SimpleNamespace(ue=harness.UE, out=out, speed=args.speed, reset=lambda: harness.start_run(RESET_SEED))
    summary = {}
    for name in args.only.split(","):
        t0 = time.time()
        print(f"== {name}")
        try:
            result = importlib.import_module(SUITES[name]).suite(ctx)
        except Exception as e:  # a crashed suite is a failed suite; the others still run
            print(f"  crashed: {e!r}")
            result = {"ok": False, "checks": 0, "problems": [f"crashed: {e!r}"], "files": {}}
        summary[name] = {**result, "seconds": round(time.time() - t0)}
        try:
            harness.set_speed(args.speed)  # what a crashed suite may have left
        except Exception:
            pass
    if not args.keep_editor:
        harness.kill_editor()
    (out / "summary.json").write_text(json.dumps({"speed": args.speed, "suites": summary}, indent=1))
    print(f"\nspeed {args.speed}x, results in {out}")
    for name, r in summary.items():
        print(f"  {'PASS' if r['ok'] else 'FAIL'}  {name:10} {r['seconds']:6.0f} s  {r['checks']:4} checks")
        if not r["ok"]:
            for problem in r["problems"][:5]:
                print(f"        {problem}")
    print(f"  total {time.time() - t_all:.0f} s")
    sys.exit(0 if all(r["ok"] for r in summary.values()) else 1)


if __name__ == "__main__":
    main()
