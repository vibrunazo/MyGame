"""Shared test plumbing: start the editor and PIE once, start runs with a seed, and wait on the game's clock.

Every scripted wait in the suites is in game seconds (`wait_game`), so the suites run at whatever speed the game is
set to (`set_speed`, global time dilation re-applied after each floor load). The game is meant to be frame rate and
speed agnostic: a suite that changes outcome at another speed has found a bug (or a timing-sensitive test).
"""
import subprocess
import time
from pathlib import Path

from mcp_client import SESSION_FILE, call_tool, tool

ROOT = Path(__file__).resolve().parent.parent
EDITOR = Path(r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe")
APP = "EditorToolset.EditorAppToolset"
# an unfocused editor throttles itself to a few frames a second; tests need every frame
NO_THROTTLE = "-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.EditorPerformanceSettings]:bThrottleCPUWhenNotForeground=False"


def kill_editor():
    subprocess.run(["taskkill", "/IM", "UnrealEditor.exe", "/F"], capture_output=True)
    time.sleep(3)


def start_editor(log="tests", render=False, seed=None, timeout=600):
    """A fresh headless editor (rendering offscreen when `render`) with PIE running on LevelPersistent."""
    kill_editor()
    args = [str(EDITOR), str(ROOT / "MyGame.uproject"), "-unattended", "-nosplash",
            "-RenderOffscreen" if render else "-nullrhi", NO_THROTTLE, f"-log={log}.log"]
    if seed:
        args.append(f"-Seed={seed}")
    subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    # up when its MCP server answers (a log line could be a previous run's)
    SESSION_FILE.write_text("")
    t0 = time.time()
    while True:
        try:
            call_tool(APP, "StartPIE", {"options": {"bSimulate": False, "playMode": "PlayMode_InViewPort", "warmupSeconds": 6}})
            break
        except (OSError, RuntimeError, AttributeError):
            if time.time() - t0 > timeout:
                raise
            SESSION_FILE.write_text("")
            time.sleep(5)


def game_time():
    return tool("game_time")["t"]


def wait_game(seconds, poll=0.02):
    """Waits `seconds` of game time (slowed or sped up by time dilation)."""
    t0 = game_time()
    while game_time() - t0 < seconds:
        time.sleep(poll)


def set_speed(speed):
    """Runs the game `speed` times faster than real time (kept across floor loads)."""
    tool("set_game_speed", speed=float(speed))


def start_run(seed, timeout=60):
    """A new run with this seed (the game instance's StartNewRun, as the menu does), loaded and unfrozen."""
    from level_regression import wait_loaded
    tool("start_run", seed=int(seed))
    t0 = time.time()
    while time.time() - t0 < timeout:
        try:
            if tool("run_state")["seed"] == seed and tool("run_state")["player"]:
                break
        except RuntimeError:
            pass  # between maps
        time.sleep(0.5)
    return wait_loaded()
