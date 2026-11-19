"""Ass Step 7's world checks (../Ass/Docs/Step6.md 4.8), on MyGame's own maps, before any pattern depends on them.

The `world` suite of `unreal-agent test` (`suite(ctx)`):

- The spot search: the plugin's Move To Spot search (its mover's grid projected on the navigation mesh, each point's
  distance and sight, AssSpots' score band) against the engine running EQS_FindAttackSpot, as the same enemy with the
  player as its target, on seed 6032's floor: the start room with the enemy standing inside the grid, then the middle
  and corners of every room, at three facings. The two bands (the points the query's best-25% pick chooses among) must
  hold the same points, to the centimetre. The engine runs a copy of the query in memory with its scores left as that
  pick sees them (its All Matching mode would rescale them; MyGameEditorLibrary.CompareSpotSearchForTest).
- The token arbiter: a UAssTokenArbiterComponent (one token, kept until death) put on the player beside its aggro list
  and asked every question the tree asks of the list (an addition is a request, IsInAggroList is Held Or Free), through
  the AI suite's token scenarios (the tie and the stunned holder) and the decision matrix's rows of the base enemy (the
  token free, its own, another's). Every answer must agree, but where a holder was removed without dying (each
  scenario's cleanup destroys its enemies): the list keeps such a holder until garbage collection and the arbiter lets
  it go at once, the difference the arbiter is for (Plan.md Step 7 item 2). Those answers are counted apart. Once
  every enemy runs its pattern, no tree asks the list: that comparison says so and is skipped, until the list goes.
"""
import json

import ability_regression as ar
import ai_behaviour as ab
import mygame
import strict
from mygame import tool

QUERY = "/Game/Blueprints/AI/EQS_FindAttackSpot"
# Move To Spot's fields with MyGame's values (Docs/Step6.md 4.4), as its patterns will carry them
SPOT = {"behind": 300.0, "half_extent": 500.0, "spacing": 100.0, "distance_weight": 1.5, "sight_score": 1.0, "ratio": 0.75}
FACINGS = (0.0, 135.0, 270.0)
# where the player stands in each room, from its cell's centre: the middle and towards each corner
OFFSETS = ((0.0, 0.0), (450.0, 700.0), (-450.0, 700.0), (450.0, -700.0), (-450.0, -700.0))


TILE = (1400.0, 2000.0)  # a tile's size (the glossary); room_bounds gives each room's cell in the grid


def spot_places():
    """(label, x, y, z) for the player: inside every room of the floor, standing on its floor (a pit is left out, so
    the player never falls out of the world)."""
    out, pits = [], []
    for room in tool("room_bounds"):
        cx, cy = room["cell"][0] * TILE[0], room["cell"][1] * TILE[1]
        for dx, dy in OFFSETS:
            label = f"{room['map']} {dx:+.0f},{dy:+.0f}"
            floor = tool("floor_height", x=cx + dx, y=cy + dy, from_z=room["floor_z"][1] + 300.0)
            if not floor.get("hit") or floor["z"] < room["floor_z"][0] - 50.0:
                pits.append(label)
                continue
            out.append((label, cx + dx, cy + dy, floor["z"] + 100.0))
    if pits:
        print(f"  no floor at {pits}")
    return out


def spot_check(enemy, label, x, y, z, yaw):
    tool("teleport_player", x=x, y=y, z=z)
    tool("set_player_facing", yaw=yaw)
    ar.freeze_enemies()  # the rooms spawn theirs as the player arrives
    ar.wait_game(0.2)
    r = tool("compare_spot_search", actor_path=enemy, query_path=QUERY, spot=json.dumps(SPOT))
    same = not r["only_engine"] and not r["only_plugin"] and r["engine"]
    line = (f"{label} yaw {yaw:.0f}: band {len(r['engine'])} engine, {len(r['plugin'])} plugin, of "
            f"{r['engine_points']} and {r['plugin_points']} points")
    if not same:
        line += f"; only the engine's {r['only_engine'][:4]}, only the plugin's {r['only_plugin'][:4]}"
    return same, line, r


def token_scenarios():
    yield from ab.tie_scenarios()
    yield from ab.holder_scenarios()
    yield from (s for s in ab.matrix_scenarios() if s["name"].startswith("enemy1."))


def suite(ctx):
    mygame.use(ctx)
    ctx.reset()
    player = tool("pie_player_state")
    ar.PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    problems, spots, tokens = [], {}, {}

    # the spot search, by a frozen enemy standing inside the grid in the start room, then far from the player
    def fresh_enemy():
        mygame.wait_until(lambda: tool("run_state")["player"], timeout=30.0, what="the player")
        ar.freeze_enemies()
        for a, v in (("MaxHealth", 100000.0), ("Health", 100000.0), ("Defense", 1000.0)):
            tool("set_player_attribute", attribute=a, value=v)
        return tool("spawn_enemy", class_path=strict.CHARS["enemy1"], x=strict.X, y=strict.Y, z=ar.stand_z(), yaw=180.0)["path"]

    enemy, fell = fresh_enemy(), []
    places = [("start room, the enemy in the grid", strict.CLOSE - 300.0, strict.Y, ar.stand_z())] + spot_places()
    for label, x, y, z in places:
        for yaw in FACINGS:
            r = None
            for _ in range(2):
                try:
                    same, line, r = spot_check(enemy, label, x, y, z, yaw)
                    break
                except Exception as e:  # the player was lost there: a fresh run, the place tried once more
                    if "no player" not in str(e):
                        raise
                    ctx.reset()
                    enemy = fresh_enemy()
            if r is None:
                fell.append(f"{label} {yaw:.0f}")
                continue
            spots[f"{label} {yaw:.0f}"] = r
            print(("  " if same else "  DIFFERS ") + line)
            if not same:
                problems.append("spot " + line)
    if fell:
        print(f"  skipped, the player lost twice: {fell}")
    if len(spots) < len(places) * len(FACINGS) * 0.8:
        problems.append(f"spot: only {len(spots)} places compared, the player lost at {fell}")
    tool("destroy_actor", actor_path=enemy)

    # the token arbiter beside the aggro list
    base = strict.base_attributes(tool("pie_player_state"))
    ar.DUMMY[0] = {"path": strict.DUMMY_PATH}
    questions = 0
    for sc in token_scenarios():
        tool("token_shadow", on=True)
        try:
            strict.run(sc, base)
        finally:
            shadow = tool("token_shadow", on=False)
        tokens[sc["name"]] = shadow
        questions += shadow["questions"]
        if shadow["mismatches"]:
            problems.append(f"token {sc['name']}: {shadow['mismatches'][:3]}")
            print(f"  DIFFERS {sc['name']}: {shadow['mismatches'][:3]}")
    print(f"  token: {len(tokens)} scenarios, {questions} questions, "
          f"{sum(t['additions'] for t in tokens.values())} additions, {sum(len(t['mismatches']) for t in tokens.values())} mismatches; "
          f"{sum(len(t['removed_holder']) for t in tokens.values())} answers apart because the scenario's cleanup removed "
          f"the holder without a death (the list keeps it, the arbiter lets it go: Plan.md Step 7 item 2)")
    if questions == 0:
        # every enemy runs its pattern (Ass Step 7 item 4): no tree asks the aggro list, which leaves with item 5
        print("  token: no tree asks the aggro list now (the enemies run their patterns): nothing to compare, skipped")

    (ctx.out / "world.json").write_text(json.dumps({"spots": spots, "tokens": tokens}, indent=1))
    return {"ok": not problems, "checks": len(spots) + len(tokens), "problems": problems, "files": {"world.json": None}}
