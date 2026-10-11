"""Smoke test for the packaged Development build (RunUAT BuildCookRun ... -archivedirectory=Saved/Packaged).

    python Tests/package_smoke.py [seconds]

Boots the main menu, then a seeded run straight into LevelPersistent, and reads the game's own log:
  menu       the main menu map loads
  run        the level builds (Grid line) and every room's room master registers
  assets     no package missing from the build (a path the cooker couldn't see: SkipPackage / Failed to find)
  clean      no Error / Fatal / Ensure lines (minus known engine noise)
  ai         every enemy is possessed by the game's AI controller (AMyCharacter::PossessedBy logs "has no game AI"
             when a Blueprint's controller is missing, and the enemy just stands there)
The editor's PIE can't catch the "assets" and "ai" cases: it loads anything from disk on demand.
"""
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GAME = ROOT / "Saved/Packaged/Windows/MyGame.exe"
LOGS = ROOT / "Saved/Packaged/Windows/MyGame/Saved/Logs"
NOISE = re.compile(r"Wintab|aqProf|PixGpu|Vtune|UnifiedError|Error test|Error with|FError")


def kill_game():
    for exe in ("MyGame-Win64-Development.exe", "MyGame.exe"):
        subprocess.run(["taskkill", "/IM", exe, "/F"], capture_output=True)


def play(name, args, ready, seconds):
    kill_game()
    name = f"{name}_{int(time.time())}"
    log = LOGS / f"{name}.log"
    proc = subprocess.Popen([str(GAME), *args, "-RenderOffscreen", "-windowed", "-resx=1280", "-resy=720", f"-log={name}.log"])
    t0 = time.time()
    while time.time() - t0 < 90 and not (log.exists() and re.search(ready, log.read_text(errors="replace"))):
        time.sleep(1)
    time.sleep(seconds)
    kill_game()
    proc.kill()
    return log.read_text(errors="replace")


def main(seconds):
    results = {}
    menu = play("smoke_menu", [], r"Took .* seconds to LoadMap\(/Game/Maps/MainMenu\)", 3)
    results["menu"] = "LoadMap(/Game/Maps/MainMenu)" in menu
    run = play("smoke_run", ["/Game/Maps/LevelPersistent", "-Seed=6032"], r"Grid:", seconds)
    rooms = len(re.findall(r"Room Level:", run))
    grid = re.search(r"Grid: (.*)", run)
    planned = grid.group(1).count("Coord:") if grid else 0
    results["run"] = bool(grid) and rooms == planned
    missing = sorted(set(re.findall(r"SkipPackage: (\S+)|Failed to find object '([^']+)'", run + menu)))
    results["assets"] = not missing
    bad = [l for l in (run + menu).splitlines() if re.search(r"Error:|Fatal|Ensure", l) and not NOISE.search(l)]
    results["clean"] = not bad
    no_ai = re.findall(r"(\S+) has no game AI", run)
    results["ai"] = not no_ai
    for k, ok in results.items():
        print(("PASS " if ok else "FAIL ") + k)
    print(f"rooms {rooms}/{planned}, enemies without AI {sorted(set(no_ai))}")
    for m in missing[:10]:
        print("  missing", m)
    for l in bad[:10]:
        print("  ", l[:200])
    print("ALL PASS" if all(results.values()) else "SOME FAILED")


if __name__ == "__main__":
    main(int(sys.argv[1]) if len(sys.argv) > 1 else 15)
