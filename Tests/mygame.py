"""MyGame's own test helpers: its hooks (the MyGameTools toolset), runs on a seed, the game's reset, floor lookups.

The suites run with `unreal-agent test`, which starts the editor and PIE on a fixed 1/60 step and hands each suite a
context (`unreal-agent guide suites`). A suite calls use(ctx) first; every call here then goes through that context's
client and waits, so nothing in MyGame names a port. `reset` is the game's known state, named in unreal-agent.toml.
"""
import json
import time
from pathlib import Path

MYGAME_TOOLS = "mygame_tools.toolset.MyGameTools"
BASE = Path(__file__).resolve().parent / "baselines"
RESET_SEED = 6032  # the known state: a new run on this seed, loaded and unfrozen
_ctx = None


def use(ctx):
    """Sends every call below through this context (the runner's, or one a hand-run script builds)."""
    global _ctx
    _ctx = ctx


def reset(ctx):
    """The game's known state, called by the runner: a new run on seed 6032, on its own floor."""
    use(ctx)
    return start_run(RESET_SEED)


def call_tool(toolset, name, arguments=None):
    return _ctx.ue.call_tool(toolset, name, arguments or {})


def tool(name, **arguments):
    """Calls one of MyGame's hooks (Content/Python/mygame_tools)."""
    return call_tool(MYGAME_TOOLS, name, arguments)


def wait_game(seconds):
    """Waits `seconds` of game time, inside the engine."""
    _ctx.wait_game(seconds)


def game_time():
    return tool("game_time")["t"]


def set_speed(speed):
    """Time dilation on top of the fixed step, kept across floor loads."""
    tool("set_game_speed", speed=float(speed))


def wait_loaded(timeout=30.0):
    """Until the tiles finished streaming in (the game unfrozen) and stop changing (a condition, wall clock as the
    limit: the game is frozen while it loads)."""
    t0, last = time.time(), None
    while time.time() - t0 < timeout:
        s = tool("level_state")
        key = (len(s["tiles"]), len(s["tile_masters"]), s["time_dilation"], json.dumps(s.get("props"), sort_keys=True))
        if s["time_dilation"] > 0.0 and key == last:
            return s
        last = key
        time.sleep(1.0)
    return tool("level_state")


def start_run(seed, timeout=60, check_floor=True):
    """A new run with this seed (the game instance's StartNewRun, as the menu does), loaded and unfrozen. Fails when
    the floor isn't the one the seed's level baseline recorded, unless `check_floor` is off (the level suite compares
    the floor itself)."""
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
    baseline = BASE / f"level_seed_{seed}.json"
    if check_floor and baseline.exists():
        want = sorted(sorted(map(list, r)) for r in json.loads(baseline.read_text())["rooms"])
        got = sorted(sorted(map(list, r)) for r in state["rooms"])
        if got != want:
            raise RuntimeError(f"start_run({seed}) built another floor: rooms {got}, the seed's baseline has {want}")
    return state


def floor_z(x, y, from_z=500.0, tries=3):
    """The floor's height at (x, y), traced straight down through world geometry from `from_z`. A spot with no floor
    is tried again a second later (the game may be frozen while a tile streams in), then fails with what the level
    looked like."""
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
