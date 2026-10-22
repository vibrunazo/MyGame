"""Game loop regression for Phase 5: clear a floor, carry the run over, die, start again, go through the menu.

Needs the editor running with PIE started on LevelPersistent and a fixed seed (`-Seed=N`).

    python Tests/loop_regression.py run out.json

Checks (each prints PASS/FAIL, the JSON keeps the states seen):
  floor_clear   killing the boss room and touching the flag travels to floor 2, one room bigger
  carry_over    health, items and learned moves (a book's move included) survive the travel
  no_control    after the flag the player can't move
  game_over     dying shows the game over screen
  new_run       a new run is floor 1 again: full health, no items, starting moves only
  menu          quitting goes to the main menu, and a new run from there loads the level
  input_mode    every level a run plays starts in Game Only input (injected input skips Slate focus, so a level
                left in the menu's UI Only mode would pass every other check while real keys and pads do nothing)
"""
import json
import sys
import time

from harness import wait_game
from harness import tool
from level_regression import wait_loaded

BOOK, ITEM = "/Game/Blueprints/Items/DA_LearnTat", "/Game/Blueprints/Items/DA_ItemFireHands"
results = {}


def check(name, ok, detail=""):
    results[name] = {"pass": bool(ok), "detail": detail}
    print(("PASS " if ok else "FAIL ") + name, detail)


def wait_for(cond, timeout=40.0, step=1.0):
    t0, state = time.time(), None
    while time.time() - t0 < timeout:
        try:
            state = tool("run_state")
            if cond(state):
                return state
        except RuntimeError:
            pass  # between maps
        time.sleep(step)
    return state


def room_of(at, size):
    return [round(at[0] / size[0]), round(at[1] / size[1])]


def clear_floor(level):
    """Kill the boss room's enemies, stand on the flag; returns the player's location just after."""
    size = level["tile_size"]
    goal = tool("list_actors", class_path="/Script/MyGame.Goal")[0]["location"]
    room = room_of(goal, size)
    tool("set_player_attribute", attribute="Defense", value=1000.0)
    tool("teleport_player", x=room[0] * size[0], y=room[1] * size[1], z=goal[2] + 150)
    wait_game(1.5)
    for c in tool("list_characters"):
        if c["team"] == 1 and c.get("health", 0) > 0 and room_of(c["location"], size) == room:
            tool("kill_character", actor_path=c["path"])
    wait_game(2)
    tool("teleport_player", x=goal[0], y=goal[1], z=goal[2] + 100)
    wait_game(1.0)


def player_pos():
    return next(c["location"] for c in tool("list_characters") if c["team"] == 0)


def run(out):
    run_all()
    with open(out, "w") as f:
        json.dump(results, f, indent=1)
    print("ALL PASS" if all(v.get("pass", True) for v in results.values() if isinstance(v, dict) and "pass" in v) else "SOME FAILED")


def run_all():
    results.clear()
    floor1 = wait_loaded()
    start = tool("run_state")
    results["start"] = start
    starting_moves = start["abilities"]

    # a run in progress: a book learned, an item picked up, some damage taken
    tool("learn_item_abilities", item_asset_path=BOOK)
    tool("give_item", item_asset_path=ITEM)
    tool("set_player_attribute", attribute="Health", value=70.0)
    before = tool("run_state")

    # the HUD's buttons follow the device: keys on keyboard (from the mapping context), pictures on a gamepad
    shown = {dev: tool("hud_hotkeys", device=dev)["buttons"] for dev in ("keyboard", "gamepad")}
    visible = [n for n, b in shown["keyboard"].items() if b["visible"] != "HIDDEN"]
    keys_ok = all(shown["keyboard"][n]["shows"] == "keys" and shown["keyboard"][n]["keys"] for n in visible)
    pads_ok = all(b["shows"] == "gamepad" for b in shown["gamepad"].values())
    check("hud_hotkeys", visible and keys_ok and pads_ok,
          {shown["keyboard"][n]["slot"]: shown["keyboard"][n]["keys"] for n in visible})

    clear_floor(floor1)
    p0 = player_pos()
    tool("move_input", forward=1.0, right=1.0)
    wait_game(1.5)
    tool("move_input", forward=0.0, right=0.0)
    p1 = player_pos()
    moved = ((p1[0] - p0[0]) ** 2 + (p1[1] - p0[1]) ** 2) ** 0.5
    check("no_control", moved < 1.0, f"moved {moved:.0f} after the flag")

    floor2_state = wait_for(lambda s: s["floor"] == 2 and s["player"])
    floor2 = wait_loaded()
    results["floor2"] = floor2_state
    check("floor_clear", floor2_state and floor2_state["floor"] == 2 and len(floor2["tiles"]) == len(floor1["tiles"]) + 1,
          f'floor {floor2_state and floor2_state["floor"]}, tiles {len(floor1["tiles"])} -> {len(floor2["tiles"])}')
    check("carry_over", floor2_state["items"] == before["items"] and floor2_state["abilities"] == before["abilities"]
          and abs(floor2_state["player"]["Health"] - 70.0) < 0.5,
          f'items {floor2_state["items"]}, health {floor2_state["player"]["Health"]}, '
          f'moves kept {floor2_state["abilities"] == before["abilities"]}')

    # death
    player = next(c["path"] for c in tool("list_characters") if c["team"] == 0)
    tool("kill_character", actor_path=player)
    over = wait_for(lambda s: "WBP_GameOver_C" in s["widgets"], timeout=15)
    results["game_over"] = over
    check("game_over", over and "WBP_GameOver_C" in over["widgets"], f'widgets {over and over["widgets"]}')

    # new run from the game over screen
    tool("run_action", action="start_new_run")
    time.sleep(3)  # the floor reloads: real time, the game clock restarts
    fresh = wait_for(lambda s: s["player"] and s["map"] == "LevelPersistent")
    wait_loaded()
    fresh = tool("run_state")
    results["new_run"] = fresh
    check("new_run", fresh["floor"] == 1 and fresh["items"] == [] and fresh["abilities"] == starting_moves
          and fresh["player"]["Health"] == fresh["player"]["MaxHealth"],
          f'floor {fresh["floor"]}, items {fresh["items"]}, health {fresh["player"]["Health"]}, '
          f'starting moves {fresh["abilities"] == starting_moves}')

    # menu and back
    tool("run_action", action="quit_to_main_menu")
    menu = wait_for(lambda s: s["map"] == "MainMenu")
    results["menu"] = menu
    tool("run_action", action="start_new_run")
    back = wait_for(lambda s: s["map"] == "LevelPersistent" and s["player"])
    check("menu", menu and menu["map"] == "MainMenu" and back and back["map"] == "LevelPersistent",
          f'{menu and menu["map"]} -> {back and back["map"]}, widgets in menu {menu and menu["widgets"]}')
    modes = {"floor 2": floor2_state["input_mode"], "game over": over and over["input_mode"],
             "new run": fresh["input_mode"], "main menu": menu and menu["input_mode"], "run from menu": back and back["input_mode"]}
    check("input_mode", all((modes[k] or "").startswith("Game Only") for k in ("floor 2", "new run", "run from menu")),
          str({k: v and v.split(" (")[0] for k, v in modes.items()}))

    return results


if __name__ == "__main__":
    run(sys.argv[2])
