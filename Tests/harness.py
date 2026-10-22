"""Shared test plumbing: start the editor and PIE once, start runs with a seed, and wait on the game's clock.

Every scripted wait in the suites is in game seconds (`wait_game`), so the suites run at whatever speed the game is
set to (`set_speed`, global time dilation re-applied after each floor load). The game is meant to be frame rate and
speed agnostic: a suite that changes outcome at another speed has found a bug (or a timing-sensitive test).
"""
import time
from pathlib import Path

from unreal_agent_tools import Client, editor

ROOT = Path(__file__).resolve().parent.parent
PROJECT = ROOT / "MyGame.uproject"
APP = "EditorToolset.EditorAppToolset"
MYGAME_TOOLS = "mygame_tools.toolset.MyGameTools"
# the editor's MCP server (port 18765, or UNREAL_MCP_PORT): one client for every suite
UE = Client()


def call_tool(toolset, name, arguments=None):
    return UE.call_tool(toolset, name, arguments)


def tool(name, **arguments):
    """Calls one of this project's MyGameTools hooks (Content/Python/mygame_tools)."""
    return UE.call_tool(MYGAME_TOOLS, name, arguments)


def kill_editor():
    editor.stop(PROJECT)


def start_editor(log="tests", render=False, seed=None, timeout=600):
    """A fresh headless editor (rendering offscreen when `render`) with PIE running on LevelPersistent."""
    editor.start(PROJECT, port=UE.port, render=render, log=log, args=(f"-Seed={seed}",) if seed else (), timeout=timeout)
    for attempt in range(10):  # the editor can still be finishing its startup
        try:
            call_tool(APP, "StartPIE", {"options": {"bSimulate": False, "playMode": "PlayMode_InViewPort", "warmupSeconds": 6}})
            return
        except RuntimeError:
            if attempt == 9:
                raise
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
