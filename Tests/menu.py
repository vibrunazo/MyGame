"""Menu and input regression: the menus work with real keys and a gamepad, and every run they start takes the
player's input.

The `menu` suite of `unreal-agent test` (`suite(ctx)`). PIE starts on the main menu map, in a rendering editor: keys
reach UMG and UI Only menus only through Slate, as a real keyboard's do (ctx.key). Once with the keyboard, then with a
gamepad, it goes:
  menu         the main menu shows, in UI Only input
  start        confirm starts a run (keys only, no clicks), and the level loads
  move         the run is in Game Only input and a held W (the stick, on the gamepad trip) moves the player (the cooked
               game's dead keyboard was a level left in the menu's UI Only mode)
  pause        the pause key shows the pause menu and pauses the game; the pause key again resumes it
  back         pause, down to the pause menu's Exit button, confirm: back on the main menu, in UI Only input
The gamepad's trip starts from the menu the keyboard's trip came back to, so its run is a second trip through the menu,
where the bug lived; it moves with the left stick pushed forward.
"""
import json
import time

import mygame
from mygame import tool, wait_loaded

MENU_MAP, RUN_MAP = "MainMenu", "LevelPersistent"
MENU_WIDGET, PAUSE_WIDGET = "WBP_MainMenu_C", "WBP_PauseMenu_C"
DEVICES = [
    {"name": "keyboard", "confirm": "Enter", "down": "Down", "pause": "Escape", "move": "W"},
    {"name": "gamepad", "confirm": "Gamepad_FaceButton_Bottom", "down": "Gamepad_DPad_Down", "pause": "Gamepad_Special_Right",
     "move": "Gamepad_LeftY=1.0"},
]
MOVE_HOLD, MIN_MOVE = 0.5, 50.0
EXIT = "ExitButton in WBP_PauseMenu_C"  # the pause menu's button back to the main menu


def input_mode(ctx):
    """The player controller's input mode, read by reflection (the plugin's InputMode on a PlayerController)."""
    found = ctx.objects("PlayerController", ["InputMode"])  # [{name, class, path, properties}]
    return next((o["properties"]["InputMode"] for o in found or [] if (o.get("properties") or {}).get("InputMode")), None)


def wait_state(cond, timeout=40.0):
    """The run state once `cond` holds (a condition, wall clock as the limit: levels load in real time)."""
    t0, state = time.time(), None
    while time.time() - t0 < timeout:
        try:
            state = tool("run_state")
            if cond(state):
                return state
        except RuntimeError:
            pass  # between maps
        time.sleep(0.25)
    return state


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

    def player_xy(self):
        return tool("pie_player_state")["location"][:2]

    def run(self):
        dev = self.dev
        s = wait_state(lambda s: s["map"] == MENU_MAP and MENU_WIDGET in s["widgets"])
        mode = input_mode(self.ctx)
        if not self.check("menu", s and s["map"] == MENU_MAP and MENU_WIDGET in s["widgets"] and str(mode).startswith("UI Only"),
                          {"map": s and s["map"], "widgets": s and s["widgets"], "input_mode": mode}):
            return
        r = self.press(dev["confirm"])
        s = wait_state(lambda s: s["map"] == RUN_MAP and s["player"])
        if not self.check("start", s and s["map"] == RUN_MAP and s["player"],
                          {"confirm": r, "map": s and s["map"]}):
            return
        wait_loaded()
        mode = input_mode(self.ctx)
        before = self.player_xy()
        r = self.press(dev["move"], hold=MOVE_HOLD)
        moved = round(sum((a - b) ** 2 for a, b in zip(self.player_xy(), before)) ** 0.5, 1)
        self.check("move", str(mode).startswith("Game Only") and moved >= MIN_MOVE,
                   {"input_mode": mode, "moved": moved, "key": r})
        r = self.press(dev["pause"])
        s = wait_state(lambda s: PAUSE_WIDGET in s["widgets"], timeout=10)
        paused = self.ctx.clock().get("paused")
        r2 = self.press(dev["pause"])
        running = not self.ctx.clock().get("paused")
        self.check("pause", s and PAUSE_WIDGET in s["widgets"] and paused and running,
                   {"pause_menu": bool(s and PAUSE_WIDGET in s["widgets"]), "paused": paused, "resumed": running,
                    "pause_key": r, "resume_key": r2})
        self.press(dev["pause"])
        wait_state(lambda s: PAUSE_WIDGET in s["widgets"], timeout=10)
        on_exit = self.to_exit()
        r = self.press(dev["confirm"])
        s = wait_state(lambda s: s["map"] == MENU_MAP and MENU_WIDGET in s["widgets"])
        mode = input_mode(self.ctx)
        self.check("back", on_exit and s and s["map"] == MENU_MAP and str(mode).startswith("UI Only"),
                   {"moved_to_exit": on_exit, "confirm": r, "map": s and s["map"], "input_mode": mode})


def suite(ctx):
    """Both devices' trips through the menus, each step checked."""
    mygame.use(ctx)
    trips = []
    for device in DEVICES:
        trip = Trip(ctx, device)
        trip.run()
        trips.append(trip)
        if trip.problems:
            break  # the next device starts where this one left the menus
    (ctx.out / "menu.json").write_text(json.dumps([t.record for t in trips], indent=1))
    problems = [p for t in trips for p in t.problems]
    for p in problems:
        print("  " + p[:300])
    return {"ok": not problems and len(trips) == len(DEVICES), "checks": sum(t.checks for t in trips),
            "problems": problems or ([] if len(trips) == len(DEVICES) else ["a trip didn't run"]), "files": {"menu.json": None}}
