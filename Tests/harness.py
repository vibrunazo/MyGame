"""Shared test plumbing: start the editor and PIE once, start runs with a seed, and wait on the game's clock.

Every scripted wait in the suites is in game seconds (`wait_game`), so the suites run at whatever speed the game is
set to (`set_speed`, global time dilation re-applied after each floor load). The game is meant to be frame rate and
speed agnostic: a suite that changes outcome at another speed has found a bug (or a timing-sensitive test).
"""
import json
import time
from pathlib import Path

from unreal_agent_tools import Client, editor

ROOT = Path(__file__).resolve().parent.parent
PROJECT = ROOT / "MyGame.uproject"
APP = "EditorToolset.EditorAppToolset"
MYGAME_TOOLS = "mygame_tools.toolset.MyGameTools"
# the editor's MCP server (port 18765, or UNREAL_MCP_PORT): one client for every suite; a suite points it at its
# ctx.ue with use()
UE = Client()


def use(client):
    """Sends every hook call to `client`: a suite's ctx.ue, the editor its runner started."""
    global UE
    UE = client


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


def floor_z(x, y, from_z=500.0, tries=3):
    """The floor's height at (x, y), traced straight down through world geometry from `from_z`. A spot with no floor
    is tried again a second later (wall clock: the game may be frozen while a tile streams in), then fails with what
    the level looked like, instead of a KeyError further on."""
    for attempt in range(tries):
        hit = tool("floor_height", x=float(x), y=float(y), from_z=float(from_z))
        if hit.get("hit"):
            if attempt:
                print(f"  note: the floor at ({x:.0f}, {y:.0f}) was found only on try {attempt + 1}")
            return hit["z"]
        time.sleep(1.0)
    s = tool("level_state")
    raise RuntimeError(f"no floor at ({x:.0f}, {y:.0f}) below z {from_z:.0f} after {tries} tries: "
                       f"{len(s['tiles'])} tiles, time dilation {s['time_dilation']}, run {tool('run_state')}")


def set_speed(speed):
    """Runs the game `speed` times faster than real time (kept across floor loads)."""
    tool("set_game_speed", speed=float(speed))


def start_run(seed, timeout=60, check_floor=True):
    """A new run with this seed (the game instance's StartNewRun, as the menu does), loaded and unfrozen. Fails when
    the floor isn't the one the seed's level baseline recorded, unless `check_floor` is off (the level suite compares
    the floor itself)."""
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
    state = wait_loaded()
    # every suite's numbers assume the seed's own floor: fail here, clearly, if another one was built
    baseline = Path(__file__).parent / "baselines" / f"level_seed_{seed}.json"
    if check_floor and baseline.exists():
        want = sorted(sorted(map(list, r)) for r in json.loads(baseline.read_text())["rooms"])
        got = sorted(sorted(map(list, r)) for r in state["rooms"])
        if got != want:
            raise RuntimeError(f"start_run({seed}) built another floor: rooms {got}, the seed's baseline has {want}")
    return state
