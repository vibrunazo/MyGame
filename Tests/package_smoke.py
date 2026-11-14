"""Smoke test for the packaged Development build: the `smoke` suite of `unreal-agent test --on packaged`.

`unreal-agent package` builds the game into Saved/Packaged/Windows; this suite (`suite(ctx)`) runs the cooked game
twice through ctx.run_game (its own executable, offscreen, a saved folder of its own) and reads its log:
  menu       the main menu map loads
  run        a seeded run straight into LevelPersistent: the level builds (the Tiles line), every room streams in
             (Tiles ready) and every room's room master registers
  assets     no package missing from the build (a path the cooker couldn't see: SkipPackage / Failed to find)
  clean      no Error / Fatal / Ensure lines (minus known engine noise)
  ai         every enemy is possessed by the game's AI controller (AMyCharacter::PossessedBy logs "has no game AI"
             when a Blueprint's controller is missing, and the enemy just stands there)
The editor's PIE can't catch the "assets" and "ai" cases: it loads anything from disk on demand. The menu-to-run dev
command (the `menu-to-run` suite, also run cooked) checks the menu, the input mode and the keyboard.
"""
import json
import re

NOISE = re.compile(r"Wintab|aqProf|PixGpu|Vtune|UnifiedError|Error test|Error with|FError")


def suite(ctx):
    """Both runs' logs, each check."""
    menu = ctx.run_game([], until=r"Took .* seconds to LoadMap\(/Game/Maps/MainMenu\)", timeout=120)
    run = ctx.run_game([], args=["/Game/Maps/LevelPersistent", "-Seed=6032"], until=r"Tiles ready: ", timeout=180)
    menu_log, run_log = menu.log.read_text(errors="replace"), run.log.read_text(errors="replace")
    rooms = len(re.findall(r"Room Level:", run_log))
    grid = re.search(r"Warning: Tiles: (.*)", run_log)
    planned = grid.group(1).count("Coord:") if grid else 0
    missing = sorted({a or b for a, b in re.findall(r"SkipPackage: (\S+)|Failed to find object '([^']+)'", run_log + menu_log)})
    bad = [l for l in (run_log + menu_log).splitlines() if re.search(r"Error:|Fatal|Ensure", l) and not NOISE.search(l)]
    no_ai = sorted(set(re.findall(r"(\S+) has no game AI", run_log)))
    checks = {
        "menu": ("LoadMap(/Game/Maps/MainMenu)" in menu_log, f"menu run: {menu.outcome}"),
        "run": (bool(grid) and "Tiles ready: " in run_log and rooms == planned, f"rooms {rooms}/{planned}, run: {run.outcome}"),
        "assets": (not missing, f"missing {missing[:10]}"),
        "clean": (not bad, f"{len(bad)} error lines: {[l[:160] for l in bad[:5]]}"),
        "ai": (not no_ai, f"enemies without AI {no_ai}"),
    }
    for name, (ok, detail) in checks.items():
        print(("  PASS " if ok else "  FAIL ") + name, detail)
    record = {name: {"pass": ok, "detail": detail} for name, (ok, detail) in checks.items()}
    record["logs"] = {"menu": str(menu.log), "run": str(run.log)}
    (ctx.out / "smoke.json").write_text(json.dumps(record, indent=1))
    problems = [f"{name}: {detail}" for name, (ok, detail) in checks.items() if not ok]
    return {"ok": not problems, "checks": len(checks), "problems": problems, "files": {"smoke.json": None}}
