"""Menu and input regression: the menus work with real keys and a gamepad, and every run they start takes the
player's input. Two suites run the same trips:
  menu         (pie) in a rendering editor, PIE started on the main menu map
  menu-cooked  (packaged) in the cooked game, through its bridge (ctx.start_game)
Keys reach UMG and UI Only menus only through Slate, as a real keyboard's do (ctx.key). State is read only through the
play tools (GetObjects, GetPlayWorld, the play clock), which exist in both; MyGame's editor-Python hooks don't exist in
a cooked game. Once with the keyboard, then with a gamepad, a trip goes:
  menu         the main menu shows, in UI Only input
  start        confirm starts a run (keys only, no clicks), and the floor gets ready
  move         the run is in Game Only input and a held W (the stick, on the gamepad trip) moves the player (the cooked
               game's dead keyboard was a level left in the menu's UI Only mode)
  pause        the pause key shows the pause menu and pauses the game; the pause key again resumes it
  back         pause, down to the pause menu's Exit button, confirm: back on the main menu, in UI Only input
The gamepad's trip starts from the menu the keyboard's trip came back to, so its run is a second trip through the menu,
where the bug lived.
"""
import json
import re
import time

MENU_MAP, RUN_MAP = "MainMenu", "LevelPersistent"
MENU_WIDGET, PAUSE_WIDGET = "WBP_MainMenu_C", "WBP_PauseMenu_C"
DEVICES = [
    {"name": "keyboard", "confirm": "Enter", "down": "Down", "pause": "Escape", "move": "W"},
    {"name": "gamepad", "confirm": "Gamepad_FaceButton_Bottom", "down": "Gamepad_DPad_Down", "pause": "Gamepad_Special_Right",
     "move": "Gamepad_LeftY=1.0"},
]
MOVE_HOLD, MIN_MOVE = 0.5, 50.0
EXIT = "ExitButton in WBP_PauseMenu_C"  # the pause menu's button back to the main menu


def props(ctx, cls_or_path, names=()):
    """The `properties` of every object of a class (or the one at a path), read by reflection."""
    try:
        return [o.get("properties") or {} for o in ctx.objects(cls_or_path, list(names)) or []]
    except RuntimeError:
        return []  # between maps


def world_map(ctx):
    """The play world's map: GetPlayWorld answers "<kind> <map>", and PIE prefixes the map with UEDPIE_N_."""
    try:
        return re.sub(r"^UEDPIE_\d+_", "", str(ctx.play("GetPlayWorld")).split()[-1])
    except (RuntimeError, IndexError):
        return ""


def shown(ctx, widget):
    return any(p.get("IsInViewport") for p in props(ctx, widget, ["IsInViewport"]))


def controller(ctx):
    return next(iter(props(ctx, "PlayerController", ["InputMode", "Pawn"])), {})


def input_mode(ctx):
    return controller(ctx).get("InputMode")


def player_xy(ctx):
    """The possessed pawn's location: the controller's Pawn comes back as Class'path'."""
    m = re.search(r"'([^']+)'", str(controller(ctx).get("Pawn")))
    loc = next(iter(props(ctx, m.group(1), ["K2_GetActorLocation"])), {}).get("K2_GetActorLocation") if m else None
    return (loc["x"], loc["y"]) if isinstance(loc, dict) else None


def floor_ready(ctx):
    return any(p.get("AreTilesReady") for p in props(ctx, "FloorBuilder", ["AreTilesReady"]))


def wait_until(cond, timeout=40.0):
    """Until `cond()` holds (a condition, wall clock as the limit: levels load in real time)."""
    t0 = time.time()
    while time.time() - t0 < timeout:
        if cond():
            return True
        time.sleep(0.25)
    return False


class Trip:
    def __init__(self, ctx, device):
        self.ctx, self.dev = ctx, device
        self.record = {"device": device["name"], "keys": [], "steps": {}}
        self.checks, self.problems = 0, []

    def press(self, key, hold=0.1):
        r = self.ctx.key(key, hold=hold)
        r = r if isinstance(r, dict) else {"answer": r}
        self.record["keys"].append({"key": key, **r})
        return r

    def check(self, step, ok, seen):
        self.checks += 1
        self.record["steps"][step] = {"pass": bool(ok), **seen}
        if not ok:
            self.problems.append(f"{self.dev['name']} {step}: {json.dumps(seen)}")
        return ok

    def to_exit(self, tries=4):
        """Down through the pause menu until its Exit button has focus (PressKey's focus_after names the UMG widget)."""
        return any(EXIT in str(self.press(self.dev["down"]).get("focus_after", "")) for _ in range(tries))

    def run(self):
        ctx, dev = self.ctx, self.dev
        on_menu = lambda: world_map(ctx) == MENU_MAP and shown(ctx, MENU_WIDGET)
        ok = wait_until(on_menu)
        mode = input_mode(ctx)
        if not self.check("menu", ok and str(mode).startswith("UI Only"), {"map": world_map(ctx), "input_mode": mode}):
            return
        r = self.press(dev["confirm"])
        ok = wait_until(lambda: world_map(ctx) == RUN_MAP and floor_ready(ctx) and player_xy(ctx) is not None, timeout=60)
        if not self.check("start", ok, {"confirm": r, "map": world_map(ctx)}):
            return
        mode = input_mode(ctx)
        before = player_xy(ctx)
        r = self.press(dev["move"], hold=MOVE_HOLD)
        after = player_xy(ctx)
        moved = round(sum((a - b) ** 2 for a, b in zip(after, before)) ** 0.5, 1) if before and after else 0.0
        self.check("move", str(mode).startswith("Game Only") and moved >= MIN_MOVE, {"input_mode": mode, "moved": moved, "key": r})
        r = self.press(dev["pause"])
        menu_up = wait_until(lambda: shown(ctx, PAUSE_WIDGET), timeout=10)
        paused = ctx.clock().get("paused")
        r2 = self.press(dev["pause"])
        running = not ctx.clock().get("paused")
        self.check("pause", menu_up and paused and running,
                   {"pause_menu": menu_up, "paused": paused, "resumed": running, "pause_key": r, "resume_key": r2})
        self.press(dev["pause"])
        wait_until(lambda: shown(ctx, PAUSE_WIDGET), timeout=10)
        on_exit = self.to_exit()
        r = self.press(dev["confirm"])
        ok = wait_until(on_menu)
        mode = input_mode(ctx)
        self.check("back", on_exit and ok and str(mode).startswith("UI Only"),
                   {"moved_to_exit": on_exit, "confirm": r, "map": world_map(ctx), "input_mode": mode})


def trips(ctx):
    """Both devices' trips through the menus, each step checked."""
    done = []
    for device in DEVICES:
        trip = Trip(ctx, device)
        trip.run()
        done.append(trip)
        if trip.problems:
            break  # the next device starts where this one left the menus
    (ctx.out / "menu.json").write_text(json.dumps([t.record for t in done], indent=1))
    problems = [p for t in done for p in t.problems]
    for p in problems:
        print("  " + p[:300])
    return {"ok": not problems and len(done) == len(DEVICES), "checks": sum(t.checks for t in done),
            "problems": problems or ([] if len(done) == len(DEVICES) else ["a trip didn't run"]), "files": {"menu.json": None}}


def suite(ctx):
    """The `menu` pie suite: PIE starts on the main menu map, in a rendering editor."""
    return trips(ctx)


def packaged_suite(ctx):
    """The `menu-cooked` packaged suite: the cooked game, which starts on the main menu, through its bridge."""
    ctx.start_game()
    return trips(ctx)
