"""Level generator regression for Phase 4: what a seeded level looks like and how a doored room behaves.

Needs the editor running with PIE started and a fixed seed (`-Seed=N` on the editor command line).

    python Tests/level_regression.py capture out.json      # this PIE session's level
    python Tests/level_regression.py diff before.json after.json

Recorded: room maps per grid coordinate, walls (position, doorway), doors, room masters and their enemies,
enemies and props per room, and the doored-room flow: walking in locks the doors, killing the room's enemies opens them
and drops the reward.
"""
import json
import sys
import time

from mcp_client import tool


def wait_loaded(timeout=30.0):
    """Until the rooms finished streaming in (time dilation back to 1) and stop changing."""
    t0, last = time.time(), None
    while time.time() - t0 < timeout:
        s = tool("level_state")
        key = (len(s["rooms"]), len(s["room_masters"]), s["time_dilation"], json.dumps(s.get("props"), sort_keys=True))
        if s["time_dilation"] == 1.0 and key == last:
            return s
        last = key
        time.sleep(1.0)
    return tool("level_state")


def room_of(at, size):
    return [round(at[0] / size[0]), round(at[1] / size[1])]


def doored_flow(state):
    """Walk into an uncleared doored room with enemies; doors must lock. Kill them; doors must open, a reward drop."""
    size = state["room_size"]
    # walled rooms get a door on every edge that faces another room (outer edges keep their solid wall); the
    # treasure room is walled too but has no enemies, so a door plus enemies means a doored fight room
    door_edges = {(round(d["at"][0] / size[0], 1), round(d["at"][1] / size[1], 1)) for d in state["doors"]}

    def door_count(c):
        cx, cy = c
        return sum(e in door_edges for e in ((cx + 0.5, cy), (cx - 0.5, cy), (cx, cy + 0.5), (cx, cy - 0.5)))
    # its neighbors share only one of its doors each, so the doored room has the most
    candidates = sorted((m for m in state["room_masters"]
                         if door_count(m["coord"]) and state["enemies"].get(",".join(map(str, m["coord"])), 0) > 0),
                        key=lambda m: (-door_count(m["coord"]), m["coord"]))
    if not candidates:
        return {"room": None}
    room = candidates[0]["coord"]
    cx, cy = room[0] * size[0], room[1] * size[1]
    z = tool("floor_height", x=cx, y=cy, from_z=500.0)["z"] + 100.0
    tool("set_player_attribute", attribute="MaxHealth", value=1000.0)
    tool("set_player_attribute", attribute="Health", value=1000.0)
    tool("set_player_attribute", attribute="Defense", value=1000.0)
    tool("teleport_player", x=cx, y=cy, z=z)
    time.sleep(1.0)
    locked = [not d["open"] for d in tool("level_state")["doors"]]
    pickups_before = len(tool("list_actors", class_path="/Script/MyGame.Pickup"))
    for c in tool("list_characters"):
        if c["team"] == 1 and c.get("health", 0) > 0 and room_of(c["location"], size) == room:
            tool("kill_character", actor_path=c["path"])
    time.sleep(2.0)
    after = tool("level_state")
    return {"room": room, "locked_on_entry": all(locked) and bool(locked),
            "open_after_clear": all(d["open"] for d in after["doors"]),
            "reward_drops": len(tool("list_actors", class_path="/Script/MyGame.Pickup")) - pickups_before}


def capture(out):
    s = wait_loaded()
    result = {
        "room_size": s["room_size"],
        "rooms": sorted([r["coord"][0], r["coord"][1], r["map"]] for r in s["rooms"]),
        "walls": sorted([w["at"][0], w["at"][1], w["doored"]] for w in s["walls"]),
        "doors": sorted([d["at"][0], d["at"][1]] for d in s["doors"]),
        "room_masters": sorted([m["coord"][0], m["coord"][1], m["chars"]] for m in s["room_masters"]),
        "enemies": dict(sorted(s["enemies"].items())),
        "props": {k: dict(sorted(v.items())) for k, v in sorted(s.get("props", {}).items())},
        "unfrozen": s["time_dilation"] == 1.0,
    }
    result["doored_flow"] = doored_flow(s)
    print(json.dumps(result))
    with open(out, "w") as f:
        json.dump(result, f, indent=1)


def diff(a_path, b_path):
    a, b = json.load(open(a_path)), json.load(open(b_path))
    same = True
    for k in a:
        if a[k] != b.get(k):
            print(f"{k}:\n   before {a[k]}\n   after  {b.get(k)}")
            same = False
    print("same level" if same else "levels differ")


if __name__ == "__main__":
    if sys.argv[1] == "capture":
        capture(sys.argv[2])
    else:
        diff(sys.argv[2], sys.argv[3])
