"""Records the game camera through one scripted demo as an mp4, to judge a camera by eye.

Needs MyGame's editor rendering with PIE running: `unreal-agent start --render`, then `unreal-agent pie`. The script
starts the seed 6032 run itself.

    python Tests/camera_video.py out.mp4 [label] [--no-ui]

The demo (seed 6032, enemies frozen): walk right across the two-tile start room, stop and throw a fireball, fight
while turning, jump twice, then walk up through the door into the closed room above. The tools' recorder
(capture.Recording) runs the game on a fixed 1/30 step while it records, so every frame is taken and the video plays at
real speed. The frames show the HUD, as a player sees them; --no-ui leaves it out. The demo's waits are on the game
clock, inside the engine; only walk_until, which stops when the player reaches a line, polls.
"""
import sys
from pathlib import Path

from unreal_agent_tools import Client, capture
from unreal_agent_tools.testing import Context

import mygame
from mygame import floor_z, game_time, tool, wait_game, wait_until

START = (0.0, -300.0)


def press(button, hold=0.08):
    tool("press_input", button=button, pressed=True)
    wait_game(hold)
    tool("press_input", button=button, pressed=False)


def player_xy():
    loc = tool("pie_player_state")["location"]
    return loc[0], loc[1]


def walk_until(f, r, done, limit=8.0):
    """Holds the stick until done(x, y) is true (or limit game seconds pass)."""
    tool("move_input", forward=f, right=r)
    t0 = game_time()
    wait_until(lambda: done(*player_xy()) or game_time() - t0 >= limit, timeout=120, every=0.05, what="the walk")
    tool("move_input", forward=0.0, right=0.0)


def demo():
    # seed 6032: the start room is two tiles wide (0,0 and 0,1); a door at the top of tile 0,1 (x 700, y 2000) leads
    # into the closed room 1,1
    walk_until(0, 1, lambda x, y: y >= 1880)   # walk right across the start room (stopping in line with the door)
    wait_game(0.8)
    press("Cast")                               # stop and throw a fireball
    wait_game(1.2)
    for i in range(6):                          # fight: turn and punch
        tool("set_player_facing", yaw=-90.0 if i % 2 else 90.0)
        press("Punch")
        wait_game(0.45)
    for _ in range(2):                          # jumps
        tool("jump_player")
        wait_game(1.0)
    walk_until(1, 0, lambda x, y: x >= 1800)   # up through the door into the closed room: the camera eases over
    wait_game(1.5)


def main(out, label="", ui=True):
    ue = Client.for_project(Path(__file__).resolve().parent.parent)
    ctx = Context(ue=ue, out=Path(out).resolve().parent)
    mygame.use(ctx)
    mygame.start_run(6032)
    tool("learn_item_abilities", item_asset_path="/Game/Blueprints/Items/DA_LearnFireball")
    for attr, v in (("MaxHealth", 5000.0), ("Health", 5000.0), ("Defense", 1000.0), ("Mana", 1000.0)):
        tool("set_player_attribute", attribute=attr, value=v)
    tool("teleport_player", x=START[0], y=START[1], z=floor_z(*START) + 100)
    tool("set_player_facing", yaw=90.0)
    for c in tool("list_characters"):  # frozen, so the walk reaches the next room (it's a camera demo, not a fight)
        if c["team"] == 1:
            tool("set_ai_enabled", actor_path=c["path"], enabled=False, show_player=False)
    print("shaders:", ctx.wait_shaders())  # new shaders (the room darkness) finish compiling before recording starts
    with capture.Recording(ue, out, label=label or None, ui=ui) as r:
        demo()
    print(r.result)


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if a != "--no-ui"]
    main(args[0], label=args[1] if len(args) > 1 else "", ui="--no-ui" not in sys.argv)
