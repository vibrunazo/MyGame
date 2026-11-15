"""MyGame's own test helpers: its hooks (the MyGameTools toolset), runs on a seed, the game's reset, floor lookups.

The suites run with `unreal-agent test`, which starts the editor and PIE on a fixed 1/60 step and hands each suite a
context (`unreal-agent guide suites`). A suite calls use(ctx) first; every call here then goes through that context's
client and waits, so nothing in MyGame names a port. `reset` is the game's known state, named in unreal-agent.toml.
"""
import json
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
    """The play world's game time (the plugin's play clock)."""
    return _ctx.clock()["game_time"]


def objects(cls_or_path, properties=()):
    """Objects of a class (or the one at a path) in the play world, with properties read by reflection."""
    return _ctx.objects(cls_or_path, list(properties)) or []


def set_speed(speed):
    """Time dilation on top of the fixed step, kept across floor loads."""
    tool("set_game_speed", speed=float(speed))


def wait_until(condition, timeout=30.0, every=0.1, what=""):
    """The context's wait for a condition: the condition knows the game, the loop is the tools'."""
    return _ctx.wait_until(condition, timeout=timeout, every=every, what=what)


def wait_loaded(timeout=30.0):
    """The level state once the tiles have streamed in (the game unfrozen) and stopped changing between two looks a
    second apart."""
    last = [None]

    def settled():
        s = tool("level_state")
        key = (len(s["tiles"]), len(s["tile_masters"]), s["time_dilation"], json.dumps(s.get("props"), sort_keys=True))
        ready, last[0] = s["time_dilation"] > 0.0 and key == last[0], key
        return s if ready else None
    return wait_until(settled, timeout=timeout, every=1.0, what="the level to load and settle")


def start_run(seed, timeout=60, check_floor=True):
    """A new run with this seed (the game instance's StartNewRun, as the menu does), loaded and unfrozen. Fails when
    the floor isn't the one the seed's level baseline recorded, unless `check_floor` is off (the level suite compares
    the floor itself)."""
    tool("start_run", seed=int(seed))

    def started():
        s = tool("run_state")
        return s["seed"] == seed and s["player"]
    wait_until(started, timeout=timeout, every=0.5, what=f"a run on seed {seed} with its player")
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
    def hit():
        h = tool("floor_height", x=float(x), y=float(y), from_z=float(from_z))
        return h if h.get("hit") else None
    try:
        return wait_until(hit, timeout=float(tries - 1), every=1.0, what=f"a floor at ({x:.0f}, {y:.0f})")["z"]
    except TimeoutError:
        pass
    s = tool("level_state")
    raise RuntimeError(f"no floor at ({x:.0f}, {y:.0f}) below z {from_z:.0f} after {tries} tries: "
                       f"{len(s['tiles'])} tiles, time dilation {s['time_dilation']}, run {tool('run_state')}")
