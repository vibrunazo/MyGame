"""Step 0c item 2: the strict recorder's deliberate breaks, each shown to fail (Ass Plan.md, Step 0c).

The `strict-breaks` suite (manual tier). For each break of Content/Python/mygame_tools/breaks.py (at least one per
mechanic; among them a dropped class default and a stun applied before the damage) it runs the break's scenario of
Tests/strict.py three times in a row: as the content is, with the break applied in memory, and after it is undone. The
broken run must differ from the first (the gate catches it), and the third must equal the first (the scenario repeats
and the undo is complete). Nothing is saved.
"""
import json

import mygame
from mygame import tool
import ability_regression as ar
import strict


def suite(ctx):
    mygame.use(ctx)
    ctx.reset()
    player = tool("pie_player_state")
    ar.PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    base = strict.base_attributes(player)
    by_name = {sc["name"]: sc for sc in strict.scenarios()}
    breaks = tool("strict_break", which="", apply=False, list_only=True)
    problems, caught = [], {}
    for name, (scenario, change) in breaks.items():
        sc = by_name[scenario]
        before = strict.run(sc, base)
        try:
            change = tool("strict_break", which=name, apply=True)["change"]
            broken = strict.run(sc, base)
        finally:
            tool("strict_break", which="", apply=False)
        after = strict.run(sc, base)
        moved = strict.first_difference(before, broken)
        again = strict.first_difference(before, after)
        caught[name] = {"scenario": scenario, "change": change, "first_difference": moved}
        print(f"  {name} ({change}): {'caught at ' + moved[:700] if moved else 'NOT CAUGHT'}")
        if not moved:
            problems.append(f"{name}: the {scenario} recording didn't change ({change})")
        if again:
            print(f"    after the undo: {again[:700]}")
            problems.append(f"{name}: {scenario} differs after the undo: {again[:300]}")
    (ctx.out / "breaks.json").write_text(json.dumps(caught, indent=1))
    return {"ok": not problems, "checks": len(breaks), "problems": problems, "files": {}}
