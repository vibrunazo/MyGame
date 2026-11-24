"""Game loop regression for Phase 5: clear a floor, carry the run over, die, start again, go through the menu.

The `loop` suite of `unreal-agent test` (`suite(ctx)`).

Checks (each prints PASS/FAIL, the JSON keeps the states seen):
  floor_clear   killing the boss room and touching the flag travels to floor 2, one room bigger
  carry_over    health, items and learned moves (a book's move included) survive the travel
  no_control    after the flag the player can't move
  game_over     dying shows the game over screen
  new_run       a new run is floor 1 again: full health, no items, starting moves only
  menu          quitting goes to the main menu, and a new run from there loads the level
  input_mode    every level a run plays starts in Game Only input (injected input skips Slate focus, so a level
                left in the menu's UI Only mode would pass every other check while real keys and pads do nothing)
  Ass Plan.md, Step 0c item 8 (what Step 4 moves):
  summon_doored a boss's summon inside a doored room: the summoned enemies join the room, so its doors stay locked
                when the room's own enemies die and open when the summons die too
  hud_icons_all every HUD button's icon, the learned book's move included, the same on floor 2 as on floor 1
  carried_works the book's move, carried to floor 2, starts from its button there
  boss_chase_resumes  the boss's chase, its move stopped from outside its mover, moves again within a second
                (AbilityStrategySystem 0.19; before it, the boss stood punching the air until hit)
"""
import json

import mygame
from mygame import floor_z, objects, tool, wait_game, wait_loaded, wait_until

BOSS = "/Game/Blueprints/Chars/Boss1/BP_Boss1.BP_Boss1_C"
SUMMON = "/Game/Blueprints/Chars/Boss1/AT_Boss1_BossSummon1"
ALL_SLOTS = "0,1,2,3,10,11,12,20,21,22"  # EInput: the buttons and their Super and Ultra versions

BOOK, ITEM = "/Game/Blueprints/Items/DA_LearnTat", "/Game/Blueprints/Items/DA_ItemFireHands"
results = {}


def check(name, ok, detail=""):
    results[name] = {"pass": bool(ok), "detail": detail}
    print(("PASS " if ok else "FAIL ") + name, detail)


def state_when(cond):
    """A condition for wait_until: the run state, once cond(state) holds."""
    def check():
        s = tool("run_state")
        return s if cond(s) else None
    return check


def room_of(at, size):
    return [round(at[0] / size[0]), round(at[1] / size[1])]


def clear_floor(level):
    """Kill the boss room's enemies, stand on the flag; returns the player's location just after."""
    size = level["tile_size"]
    loc = objects("Goal", ["K2_GetActorLocation"])[0]["properties"]["K2_GetActorLocation"]
    goal = [loc["x"], loc["y"], loc["z"]]
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


def summon_doored(state):
    """A boss summons inside an uncleared doored room (the level suite's doored-room pick): the summons hold its doors."""
    size = state["tile_size"]
    door_edges = {(round(d["at"][0] / size[0], 1), round(d["at"][1] / size[1], 1)) for d in state["doors"]}

    def door_count(c):
        cx, cy = c
        return sum(e in door_edges for e in ((cx + 0.5, cy), (cx - 0.5, cy), (cx, cy + 0.5), (cx, cy - 0.5)))
    rooms = sorted((m for m in state["tile_masters"]
                    if door_count(m["coord"]) and state["enemies"].get(",".join(map(str, m["coord"])), 0) > 0),
                   key=lambda m: (-door_count(m["coord"]), m["coord"]))
    if not rooms:
        return {"room": None}
    room = rooms[0]["coord"]
    cx, cy = room[0] * size[0], room[1] * size[1]
    z = floor_z(cx, cy) + 100.0
    for a, v in (("MaxHealth", 1000.0), ("Health", 1000.0), ("Defense", 1000.0)):
        tool("set_player_attribute", attribute=a, value=v)
    tool("teleport_player", x=cx, y=cy, z=z)
    wait_game(1.0)
    in_room = lambda: [c for c in tool("list_characters") if c["team"] == 1 and c.get("health", 0) > 0 and room_of(c["location"], size) == room]
    own = [c["path"] for c in in_room()]
    locked = all(not d["open"] for d in tool("level_state")["doors"])
    for c in own:  # frozen, so the room's fight doesn't run while the summons are counted
        tool("set_ai_enabled", actor_path=c, enabled=False, show_player=False)
    boss = tool("spawn_enemy", class_path=BOSS, x=cx + 200.0, y=cy, z=z + 50.0, yaw=180.0)["path"]
    tool("ai_target", actor_path=boss, set_player=True)
    tool("activate_ability_template", actor_path=boss, template_path=SUMMON)
    wait_game(1.0)
    summons = [c["path"] for c in in_room() if c["path"] not in own and c["path"] != boss]
    for c in summons:
        tool("set_ai_enabled", actor_path=c, enabled=False, show_player=False)
    for c in own:
        tool("kill_character", actor_path=c)
    wait_game(2.0)
    held = all(not d["open"] for d in tool("level_state")["doors"])
    for c in summons:
        tool("kill_character", actor_path=c)
    wait_game(2.0)
    opened = all(d["open"] for d in tool("level_state")["doors"])
    tool("destroy_actor", actor_path=boss)  # a boss spawned in no room
    return {"room": room, "own": len(own), "summoned": len(summons), "locked_on_entry": locked,
            "held_by_summons": held, "open_after_summons": opened}


def boss_chase_resumes(level):
    """The floor's boss chases the player across its room; its move is stopped from outside its mover (what an invalid
    path or another system's StopMovement does); within a second it moves again and closes in. Before
    AbilityStrategySystem 0.19 the chase's command went on running with its path following idle: the boss stood,
    punching the air, until a hit cut it (seed 1401's boss in its last phase)."""
    size = level["tile_size"]
    boss = next(c for c in tool("list_characters") if c["class"] == "BP_Boss1_C")
    path, (bx, by, bz) = boss["path"], boss["location"]
    cx, cy = room_of([bx, by], size)[0] * size[0], room_of([bx, by], size)[1] * size[1]
    for a, v in (("MaxHealth", 1000.0), ("Health", 1000.0), ("Defense", 1000.0)):
        tool("set_player_attribute", attribute=a, value=v)
    # the tile's corner farthest from the boss, inside its walls
    px, py = max(((cx + sx * (size[0] / 2 - 250.0), cy + sy * (size[1] / 2 - 250.0)) for sx in (-1, 1) for sy in (-1, 1)),
                 key=lambda p: (p[0] - bx) ** 2 + (p[1] - by) ** 2)
    tool("teleport_player", x=px, y=py, z=floor_z(px, py) + 100.0)
    tool("ai_target", actor_path=path, set_player=True)
    dist = lambda: ((lambda b, p: ((b[0] - p[0]) ** 2 + (b[1] - p[1]) ** 2) ** 0.5)(
        next(c for c in tool("list_characters") if c["path"] == path)["location"], player_pos()))
    try:
        chasing = wait_until(lambda: tool("ai_move", actor_path=path)["status"] == "Moving" or None, timeout=5.0)
    except Exception:
        chasing = None
    wait_game(0.5)
    before = dist()
    stopped = tool("ai_move", actor_path=path, stop=True)["status"]
    wait_game(1.0)
    after = tool("ai_move", actor_path=path)
    closer = before - dist()
    # left to the floor clear's kill as before, from where it stood (its loot drops there, not by the flag)
    tool("ai_target", actor_path=path, clear=True)
    tool("set_ai_enabled", actor_path=path, enabled=False, show_player=False)
    tool("teleport_actor", actor_path=path, x=bx, y=by, z=bz)
    return {"chasing": bool(chasing), "stopped": stopped, "after": after, "closed_cm": round(closer), "start_cm": round(before)}


def player_pos():
    return next(c["location"] for c in tool("list_characters") if c["team"] == 0)


def suite(ctx):
    """Each step of the loop checked in turn."""
    mygame.use(ctx)
    ctx.reset()
    run_all()
    (ctx.out / "loop.json").write_text(json.dumps(results, indent=1))
    checked = {k: v for k, v in results.items() if isinstance(v, dict) and "pass" in v}
    problems = [f"{k}: {v['detail']}" for k, v in checked.items() if not v["pass"]]
    return {"ok": not problems, "checks": len(checked), "problems": problems, "files": {"loop.json": None}}


def run_all():
    results.clear()
    floor1 = wait_loaded()
    summon = summon_doored(floor1)
    results["summon"] = summon
    check("summon_doored", summon["room"] and summon["summoned"] > 0 and summon["locked_on_entry"] and summon["held_by_summons"]
          and summon["open_after_summons"], summon)
    chase = boss_chase_resumes(floor1)
    results["chase"] = chase
    check("boss_chase_resumes", chase["chasing"] and chase["stopped"] == "Idle" and chase["after"]["status"] == "Moving"
          and chase["closed_cm"] > 100, chase)
    start = tool("run_state")
    results["start"] = start
    starting_moves = start["abilities"]

    # a run in progress: a book learned, an item picked up, some damage taken
    tool("learn_item_abilities", item_asset_path=BOOK)
    tool("give_item", item_asset_path=ITEM)
    tool("set_player_attribute", attribute="Health", value=70.0)
    before = tool("run_state")

    # each starting move's button shows that move's icon (the HUD sets them from the abilities' UI data)
    icons = tool("hud_ability_icons", slots="0,1,3")
    check("hud_icons", icons == {"0": "punch", "1": "kick", "3": "jump"}, icons)
    icons_floor1 = tool("hud_ability_icons", slots=ALL_SLOTS)  # every button, the book's move (Super Kick) included

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

    floor2_state = wait_until(state_when(lambda s: s["floor"] == 2 and s["player"]), timeout=40, every=1.0, what="floor 2 with its player")
    floor2 = wait_loaded()
    results["floor2"] = floor2_state
    check("floor_clear", floor2_state and floor2_state["floor"] == 2 and len(floor2["tiles"]) == len(floor1["tiles"]) + 1,
          f'floor {floor2_state and floor2_state["floor"]}, tiles {len(floor1["tiles"])} -> {len(floor2["tiles"])}')
    check("carry_over", floor2_state["items"] == before["items"] and floor2_state["abilities"] == before["abilities"]
          and abs(floor2_state["player"]["Health"] - 70.0) < 0.5,
          f'items {floor2_state["items"]}, health {floor2_state["player"]["Health"]}, '
          f'moves kept {floor2_state["abilities"] == before["abilities"]}')
    icons_floor2 = tool("hud_ability_icons", slots=ALL_SLOTS)
    results["icons"] = {"floor1": icons_floor1, "floor2": icons_floor2}
    check("hud_icons_all", icons_floor1 == icons_floor2 and icons_floor1.get("11"), {"floor1": icons_floor1, "floor2": icons_floor2})
    # the carried book's move starts from its button on floor 2 (Super + Kick)
    tool("run_timeline", spec=json.dumps({"dummy": None, "duration": 1.0, "settle": 0.0, "settle_fixed": True,
                                          "inputs": [[0.0, "call", "press_input", {"button": "Super", "pressed": True}],
                                                     [0.1, "call", "press_input", {"button": "Kick", "pressed": True}],
                                                     [0.16, "call", "press_input", {"button": "Kick", "pressed": False}],
                                                     [0.4, "call", "press_input", {"button": "Super", "pressed": False}]]}))
    started = wait_until(lambda: (r := tool("timeline_result"))["done"] and r, timeout=60, every=0.1, what="the carried move")["result"]["abilities"]
    check("carried_works", "AT_Abilities_Tat" in started, started)

    # death
    player = next(c["path"] for c in tool("list_characters") if c["team"] == 0)
    tool("kill_character", actor_path=player)
    over = wait_until(state_when(lambda s: "WBP_GameOver_C" in s["widgets"]), timeout=15, every=1.0, what="the game over screen")
    results["game_over"] = over
    check("game_over", over and "WBP_GameOver_C" in over["widgets"], f'widgets {over and over["widgets"]}')

    # new run from the game over screen
    tool("run_action", action="start_new_run")
    # the floor reloads: the new level has no game over screen (the old one kept it, and its player, until it went)
    fresh = wait_until(state_when(lambda s: s["player"] and s["map"] == "LevelPersistent" and "WBP_GameOver_C" not in s["widgets"]),
                       timeout=40, every=1.0, what="a new run after game over")
    wait_loaded()
    fresh = tool("run_state")
    results["new_run"] = fresh
    check("new_run", fresh["floor"] == 1 and fresh["items"] == [] and fresh["abilities"] == starting_moves
          and fresh["player"]["Health"] == fresh["player"]["MaxHealth"],
          f'floor {fresh["floor"]}, items {fresh["items"]}, health {fresh["player"]["Health"]}, '
          f'starting moves {fresh["abilities"] == starting_moves}')

    # menu and back
    tool("run_action", action="quit_to_main_menu")
    menu = wait_until(state_when(lambda s: s["map"] == "MainMenu"), timeout=40, every=1.0, what="the main menu")
    results["menu"] = menu
    tool("run_action", action="start_new_run")
    back = wait_until(state_when(lambda s: s["map"] == "LevelPersistent" and s["player"]), timeout=40, every=1.0, what="a run from the menu")
    check("menu", menu and menu["map"] == "MainMenu" and back and back["map"] == "LevelPersistent",
          f'{menu and menu["map"]} -> {back and back["map"]}, widgets in menu {menu and menu["widgets"]}')
    modes = {"floor 2": floor2_state["input_mode"], "game over": over and over["input_mode"],
             "new run": fresh["input_mode"], "main menu": menu and menu["input_mode"], "run from menu": back and back["input_mode"]}
    check("input_mode", all((modes[k] or "").startswith("Game Only") for k in ("floor 2", "new run", "run from menu")),
          str({k: v and v.split(" (")[0] for k, v in modes.items()}))

    return results

