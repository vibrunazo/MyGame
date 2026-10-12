"""Records the game camera through one scripted demo and encodes it as an mp4, to judge a camera by eye.

Needs the editor running with a rendering PIE (editor_pie with -RenderOffscreen) and `-Seed=6032`.

    python Tests/camera_video.py out.mp4 [legacy] [label]

The demo (game seconds): walk right into the next room, stop and throw a fireball ahead, fight while turning,
jump twice, walk back left and stop. The game runs slowed down while every frame is captured; the frames are then
resampled to 30 fps of game time, so the video plays at real speed.
"""
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from mcp_client import tool
from level_regression import wait_loaded

SLOW = 0.25
SHOTS = Path(__file__).resolve().parent.parent / "Saved/Screenshots/WindowsEditor"
START = (0.0, -300.0)


def wait(game_seconds):
    time.sleep(game_seconds / SLOW)


def press(button, hold=0.08):
    tool("press_input", button=button, pressed=True)
    wait(hold)
    tool("press_input", button=button, pressed=False)


def move(f, r, secs):
    tool("move_input", forward=f, right=r)
    wait(secs)
    tool("move_input", forward=0.0, right=0.0)


def demo():
    move(0, 1, 3.0)                       # walk right into the next room: the camera should look ahead
    wait(0.8)
    press("Cast")                         # stop and throw a fireball ahead
    wait(1.2)
    for i in range(6):                    # fight: turn and punch
        tool("set_player_facing", yaw=-90.0 if i % 2 else 90.0)
        press("Punch")
        wait(0.45)
    for _ in range(2):                    # jumps
        tool("jump_player")
        wait(1.0)
    move(0, -1, 2.2)                      # walk back left: the look-ahead turns around
    wait(1.5)


def encode(times, name, out, label):
    frames = [SHOTS / f"{name}_{i:05d}.png" for i in range(len(times))]
    have = [(t, f) for t, f in zip(times, frames) if f.exists()]
    tmp = Path(tempfile.mkdtemp(prefix="camvid_"))
    k, j = 0, 0
    while k / 30.0 <= have[-1][0]:
        while j + 1 < len(have) and have[j + 1][0] <= k / 30.0:
            j += 1
        shutil.copyfile(have[j][1], tmp / f"o_{k:05d}.png")
        k += 1
    vf = "scale=1280:-2"
    if label:
        vf += (",drawtext=fontfile='C\\:/Windows/Fonts/arialbd.ttf':text='" + label +
               "':x=24:y=24:fontsize=36:fontcolor=white:box=1:boxcolor=black@0.5:boxborderw=10")
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", "30", "-i", str(tmp / "o_%05d.png"), "-vf", vf,
                    "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "26", "-preset", "slow", "-movflags", "+faststart", str(out)],
                   check=True)
    shutil.rmtree(tmp)
    print(f"{out}: {k} frames from {len(have)} captures ({have[-1][0]:.1f} s of game time)")


def main(out, legacy=False, label=""):
    wait_loaded()
    tool("set_legacy_camera", legacy=legacy)
    tool("learn_item_abilities", item_asset_path="/Game/Blueprints/Items/DA_LearnFireball")
    for attr, v in (("MaxHealth", 5000.0), ("Health", 5000.0), ("Defense", 1000.0), ("Mana", 1000.0)):
        tool("set_player_attribute", attribute=attr, value=v)
    z = tool("floor_height", x=START[0], y=START[1], from_z=500.0)["z"] + 100
    tool("teleport_player", x=START[0], y=START[1], z=z)
    tool("set_player_facing", yaw=90.0)
    time.sleep(3)
    name = f"camvid_{int(time.time())}"
    tool("video_frames", action="start", prefix=name, slowdown=SLOW)
    try:
        demo()
    finally:
        rec = tool("video_frames", action="stop", prefix=name, slowdown=SLOW)
    time.sleep(3)  # the last screenshots land a frame or two later
    encode(rec["times"], name, Path(out), label)


if __name__ == "__main__":
    main(sys.argv[1], legacy=len(sys.argv) > 2 and sys.argv[2] == "legacy", label=sys.argv[3] if len(sys.argv) > 3 else "")
