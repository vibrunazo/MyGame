"""Step 0c item 2: the strict recorder's scenarios (Ass Plan.md, Step 0c and 1.1).

The `strict` suite of `unreal-agent test` (`suite(ctx)`). Each scenario spawns fresh, frozen enemies of its own (never
the level's), seeds the engine's randoms as its timeline starts, and records everything its characters do with the
strict recorder: MyGameEditor's UCombatRecorder, whose header (Source/MyGameEditor/Public/CombatRecorder.h) gives the
record's format. That is every frame's transforms, montage, section and position; every ability activated, ended,
cancelled or refused; every effect applied with its magnitudes and source, in order; every tag and attribute change;
every cue, notify and spawn; and the game's own observation points (the plugin's AssTestProbe.h: each hit shape's
centre at each sample, the hits, the effects tried, projectiles refused, backslides, camera shakes, sounds and
particles played from C++).

The scenarios: the player's moves of the abilities suite; every enemy and boss ability, started directly; one per
mechanic a step of the plan moves (the Nth stun, a stun-immune target, a kill, a kill that launches, two targets in one
window, a projectile against a projectile-immune target, Attack Speed 1.5, a held attack released mid-combo, the air and
ground learn conflict, an off-axis hit); the boss's health marks, and sight and an enemy's attack with the AI on.

Fields, for comparing a recording on another skeleton (1.1): positional are the frames' x, y, z, the probes' vectors of
"shape", "hit" and "projectile_immune", the cues' locations and the spawns' locations; everything else is exact, the
times included.

Events are compared with each frame's grouped by role (by_role). Checks (the AI-on scenarios of AI_ON are recorded for
Step 0c item 4 and only noted): every ability a character or an
item grants starts somewhere and every montage something refers to plays
somewhere (three montages are referred to by nothing: Smash_Montage, PunchL_MontageEnemy2, PunchR_MontageEnemy2); and when the
committed baseline exists (BASELINE, recorded on the UE5 mannequin after the move), every scenario equals it exactly;
before it, the UE4 reference (REFERENCE, kept outside git) as 1.1 allows (TOLERANCE_CM: positions within a stated
distance, everything else exact).
"""
import json
from pathlib import Path

import mygame
from mygame import tool, wait_game, wait_until
import ability_regression as ar

ROOT = Path(__file__).resolve().parent.parent
DUMPS = ROOT / "Tests" / "baselines" / "dumps"
REFERENCE = ROOT / "Saved" / "AssReference" / "strict.json"
BASELINE = ROOT / "Tests" / "baselines" / "strict.json"  # recorded on the UE5 mannequin after the move (Step 0c)
SEED = 6032
CHARS = {
    "enemy1": "/Game/Blueprints/Chars/EMelee1/BP_Enemy1.BP_Enemy1_C",
    "enemy2": "/Game/Blueprints/Chars/EMelee2/BP_Enemy2.BP_Enemy2_C",
    "enemy3": "/Game/Blueprints/Chars/EMelee3/BP_Enemy3.BP_Enemy3_C",
    "kicker": "/Game/Blueprints/Chars/EKicker/BP_EnemyKicker.BP_EnemyKicker_C",
    "wizard": "/Game/Blueprints/Chars/EWizard/BP_EnemyWizard.BP_EnemyWizard_C",
    "boss": "/Game/Blueprints/Chars/Boss1/BP_Boss1.BP_Boss1_C",
}
ABILITY = {
    "enemy1": "/Game/Blueprints/Chars/EMelee1/GA_DamageEnemy1.GA_DamageEnemy1_C",
    "enemy2": "/Game/Blueprints/Chars/EMelee2/GA_DamageEnemy1.GA_DamageEnemy1_C",
    "enemy3": "/Game/Blueprints/Chars/EMelee3/GA_DamageEnemy2.GA_DamageEnemy2_C",
    "kicker": "/Game/Blueprints/Chars/EKicker/GA_KickerKick.GA_KickerKick_C",
    "wizard": "/Game/Blueprints/Chars/EWizard/GA_Cast1.GA_Cast1_C",
    "boss_punches": "/Game/Blueprints/Chars/Boss1/GA_BossPunches.GA_BossPunches_C",
    "boss_smash": "/Game/Blueprints/Chars/Boss1/GA_BossSmash1.GA_BossSmash1_C",
    "boss_summon": "/Game/Blueprints/Chars/Boss1/GA_BossSummon1.GA_BossSummon1_C",
    "boss_debuff": "/Game/Blueprints/Chars/Boss1/GA_BossSelfDebuff.GA_BossSelfDebuff_C",
}
# timed effects a scenario can leave on the player; the next one starts without them (their end would fall at a time the
# wall clock between scenarios decides)
LEFTOVERS = ("/Game/Abilities/GE_IsInCombat.GE_IsInCombat_C", "/Game/Abilities/GE_StunImmune.GE_StunImmune_C")
VOADORA = "/Game/Abilities/GA_Voadora.GA_Voadora_C"
AIR_PUNCH = "/Game/Abilities/GA_AirPunch.GA_AirPunch_C"
PUNCHES = "/Game/Abilities/GA_BasicPunches.GA_BasicPunches_C"
IMMUNE_TO_PROJECTILES = "/Game/Abilities/GE_ImmuneToProjectile.GE_ImmuneToProjectile_C"
# what a scenario leaves behind and is removed after it
CLEANUP = ("/Script/MyGame.MyCharacter", "/Script/MyGame.Pickup", "/Script/MyGame.MyProjectile", "/Script/AssCombat.AssHitBox",
           "/Script/MyGame.WidgetActor")
X, Y = ar.DUMMY_SPOT  # the start room's empty row
CLOSE = X - 95  # where the player stands to hit the dummy, as in the abilities suite


def enemy(role="dummy", kind="enemy1", x=X, y=Y, yaw=180.0, **attributes):
    return (role, kind, x, y, yaw, attributes)


def scenario(name, inputs, duration, enemies=(enemy(),), player_x=CLOSE, player_y=Y, player_yaw=0.0, player=None, setup=(), wait=2.5,
             seed=None, bounds=None):
    """A scenario: inputs on the game clock ([t, "call", hook, kwargs], kwargs may name a role as "@role"), the
    enemies it spawns, where the player stands, the player's attributes on top of the base ones, and hook calls
    [hook, kwargs] made before the timeline; wait is the game time let pass first (2.5 s: past every combo's reset)."""
    return {"name": name, "inputs": inputs, "duration": duration, "enemies": list(enemies), "player": (player_x, player_y, player_yaw),
            "attributes": player or {}, "setup": list(setup), "wait": wait, "seed": SEED if seed is None else seed,
            **({"bounds": bounds} if bounds else {})}


at, tap, press, move, double_tap, learn = ar.at, ar.tap, ar.press, ar.move, ar.double_tap, ar.learn


def activate(t, role, ability):
    return [at(t, "activate_ability_class", actor_path="@" + role, ability_class_path=ABILITY[ability])]


def target_player(role):
    return ["ai_target", {"actor_path": "@" + role, "set_player": True}]


def player_moves():
    """The abilities suite's scenarios, in its order, each against a fresh dummy, plus half a second to settle."""
    for name, inputs, duration, *start_room in ar.scenarios():
        if name == "fire_hands_punch":
            continue  # its burn outlasts the scenario: it goes last
        inputs = [[i[0], i[1], i[2], {k: ("@dummy" if v == DUMMY_PATH else v) for k, v in i[3].items()}] for i in inputs]
        yield scenario(name, inputs, duration + 0.5, player_x=0.0 if start_room else CLOSE)


DUMMY_PATH = "@@dummy-path@@"  # what ar.DUMMY[0]["path"] holds while the scenarios are listed


def again(t, role, ability, player_x=CLOSE):
    """The player back where it stood (a hit pushed it), then the ability once more: a landed hit makes the next
    activation play the combo's next montage."""
    return [at(t - 0.05, "teleport_player", x=player_x, y=Y, z=ar.stand_z())] + activate(t, role, ability)


def enemy_attacks():
    """Each enemy's and the boss's abilities, started directly, against the player (Defense 1, so damage lands); the
    combos twice, for their second montage."""
    hurt = {"Defense": 1.0}
    for kind in ("enemy1", "enemy2", "enemy3"):
        yield scenario(f"{kind}_attack", activate(0.0, "enemy", kind) + again(1.6, "enemy", kind), 3.5,
                       enemies=[enemy("enemy", kind)], player=hurt, setup=[target_player("enemy")])
    yield scenario("kicker_attack", activate(0.0, "enemy", "kicker"), 2.5, enemies=[enemy("enemy", "kicker")],
                   player=hurt, setup=[target_player("enemy")])
    yield scenario("wizard_cast", activate(0.0, "enemy", "wizard"), 3.0, enemies=[enemy("enemy", "wizard", x=X + 300)],
                   player=hurt, setup=[target_player("enemy")])
    for ability, inputs, duration in (("boss_punches", activate(0.0, "boss", "boss_punches") + again(3.0, "boss", "boss_punches"), 6.5),
                                      ("boss_smash", activate(0.0, "boss", "boss_smash"), 9.0),
                                      ("boss_summon", activate(0.0, "boss", "boss_summon"), 2.0),
                                      ("boss_debuff", activate(0.0, "boss", "boss_debuff"), 2.0)):
        yield scenario(ability, inputs, duration, enemies=[enemy("boss", "boss", x=X + 30)], player=hurt, setup=[target_player("boss")])


def mechanics():
    hurt = {"Defense": 1.0}
    # every third stun that lands makes the player stun-immune for 2 s (only the player counts stuns); the fourth hit
    # comes inside the immunity
    hits = activate(0.0, "enemy", "enemy1")
    for t in (1.6, 3.2, 4.8):
        hits += again(t, "enemy", "enemy1")
    yield scenario("nth_stun", hits, 6.5, enemies=[enemy("enemy", "enemy1")], player=hurt, setup=[target_player("enemy")])
    # a hit on a stun-immune target: the boss during its self debuff
    yield scenario("stun_immune_boss", activate(0.0, "boss", "boss_debuff") + tap("Punch", 0.6) + tap("Punch", 1.4), 3.5,
                   enemies=[enemy("boss", "boss", x=X + 30)], player_x=CLOSE + 30 - 15)
    yield scenario("kill", tap("Punch", 0.0), 2.5, enemies=[enemy(Health=5.0)])
    yield scenario("kill_launch", learn(0.0, "DA_LearnUppercut") + press("Super", 0.1) + tap("Punch", 0.2) + press("Super", 0.5, False),
                   3.0, enemies=[enemy(Health=5.0)])
    # two targets in one window, touched in different samples as the hand sweeps left
    yield scenario("two_targets", tap("Punch", 0.0), 2.0, enemies=[enemy(), enemy("dummy2", x=230.0, y=Y - 120)])
    # two targets touched in the same sample by the kick's first one: hit nearest first
    yield scenario("two_targets_one_sample", tap("Kick", 0.0), 2.0, enemies=[enemy("dummy", y=Y - 40), enemy("dummy2", y=Y + 40)])
    yield scenario("projectile_immune", learn(0.0, "DA_LearnFireball") + tap("Cast", 0.2), 2.5, player_x=0.0,
                   setup=[["apply_effect", {"actor_path": "@dummy", "effect_class_path": IMMUNE_TO_PROJECTILES}]])
    yield scenario("attack_speed", tap("Punch", 0.0), 2.0, player={"AttackSpeed": 1.5})
    yield scenario("held_release", press("Punch", 0.0) + press("Punch", 0.7, False), 2.5)
    # an air move learned on the punch slot replaces the air punch and keeps the ground punch; at the end both punches
    # are learned again (the kick slot keeps Voadora), so the slot is whole even if the learning went wrong
    yield scenario("air_ground_learn", [at(0.0, "learn_ability", ability_class_path=VOADORA, input="Punch")] + tap("Jump", 0.2, 0.2)
                   + tap("Punch", 0.45) + tap("Punch", 2.0) + [at(3.6, "learn_ability", ability_class_path=AIR_PUNCH, input="Punch"),
                                                       at(3.65, "learn_ability", ability_class_path=PUNCHES, input="Punch")], 4.0)
    yield scenario("off_axis", tap("Punch", 0.0), 2.0, enemies=[enemy(x=CLOSE + 65, y=Y + 65, yaw=225.0)])


def ai_on():
    """The boss's health marks (its AI off, as the abilities suite does), then sight and an enemy's attack with the AI on."""
    marks = []
    for t, health in ((0.0, 740.0), (3.0, 490.0), (6.0, 240.0)):
        marks += [at(t, "set_character_attribute", actor_path="@boss", attribute="Health", value=health),
                  at(t + 0.01, "refresh_health", actor_path="@boss")]
    yield scenario("boss_marks", marks, 9.0, enemies=[enemy("boss", "boss", x=X + 200)], player_x=CLOSE, player={"Defense": 1.0},
                   setup=[target_player("boss")])
    for name, yaw, gap in (("sight_behind_500", 0.0, 500.0), ("sight_front_900", 180.0, 900.0), ("sight_front_500", 180.0, 500.0)):
        yield scenario(name, [at(0.0, "set_ai_enabled", actor_path="@enemy", enabled=True, show_player=False)], 2.5,
                       enemies=[enemy("enemy", yaw=yaw)], player_x=X - gap)
    yield scenario("enemy_ai_attack", [at(0.0, "set_ai_enabled", actor_path="@enemy", enabled=True)], 6.0,
                   enemies=[enemy("enemy")], player={"Defense": 1.0})


# the AI's own scenarios: recorded here, held by Step 0c item 4 (Plan.md: sight and the enemy's attack "are AI checks,
# held with this item and not by the strict ability baseline"); a difference from the reference is a note. An editor
# that ran other suites first moved the end of the sight chase by a frame (the navigation's state, it seems)
AI_ON = {"sight_behind_500", "sight_front_900", "sight_front_500", "enemy_ai_attack"}


def scenarios():
    ar.DUMMY[0] = {"path": DUMMY_PATH}
    yield from player_moves()
    yield from enemy_attacks()
    yield from mechanics()
    yield from ai_on()
    yield scenario("fire_hands_punch", [at(0.0, "give_item", item_asset_path=ar.ITEMS + "DA_ItemFireHands")] + tap("Punch", 0.2), 6.0)


def resolve(value, paths):
    if isinstance(value, str) and value.startswith("@"):
        return paths[value[1:]]
    if isinstance(value, str) and value.startswith("[") and "@" in value:  # a JSON list naming roles
        return json.dumps(resolve(json.loads(value), paths))
    if isinstance(value, dict):
        return {k: resolve(v, paths) for k, v in value.items()}
    if isinstance(value, list):
        return [resolve(v, paths) for v in value]
    return value


def run(sc, base, query=None):
    """Runs one scenario and returns its record; query names roles whose frames also list what they could start."""
    for b in ("Punch", "Kick", "Cast", "Jump", "Super", "Ultra"):
        tool("press_input", button=b, pressed=False)
    tool("move_input", forward=0.0, right=0.0)
    ar.freeze_enemies()
    wait_game(sc.get("wait", 2.5))  # past every combo's reset (2.2 s), whatever the wall clock let pass
    for a, v in {**base, **sc["attributes"]}.items():
        tool("set_player_attribute", attribute=a, value=v)
    tool("reset_stun_count", actor_path=ar.PLAYER_PATH[0])
    tool("clear_aggro_list", actor_path=ar.PLAYER_PATH[0])  # the last scenario's enemies were removed, not killed
    for effect in LEFTOVERS:
        tool("remove_effect", actor_path=ar.PLAYER_PATH[0], effect_class_path=effect)
    before = {cls: set(tool("actors_of_class", class_path=cls)) for cls in CLEANUP}
    paths, spots = {"player": ar.PLAYER_PATH[0]}, {}
    for role, kind, x, y, yaw, attributes in sc["enemies"]:
        path = tool("spawn_enemy", class_path=CHARS[kind], x=x, y=y, z=ar.stand_z(), yaw=yaw)["path"]
        for a, v in {"MaxHealth": 1000.0, "Health": 1000.0, **attributes}.items():
            tool("set_character_attribute", actor_path=path, attribute=a, value=v)
        tool("refresh_health", actor_path=path)
        paths[role], spots[role] = path, (x, y, yaw)
    px, py, pyaw = sc["player"]
    tool("teleport_player", x=px, y=py, z=ar.stand_z())
    tool("set_player_facing", yaw=pyaw)
    for role, (x, y, yaw) in spots.items():
        tool("teleport_actor", actor_path=paths[role], x=x, y=y, z=ar.stand_z(), yaw=yaw)
    wait_game(0.4)
    for hook, kwargs in sc["setup"]:
        tool(hook, **resolve(kwargs, paths))
    inputs = [[i[0], i[1], i[2], resolve(i[3], paths)] for i in sc["inputs"]]
    tool("run_timeline", spec=json.dumps({"dummy": None, "duration": sc["duration"], "settle": 0.0, "settle_fixed": True,
                                          "inputs": inputs, "record": paths, "seed": sc.get("seed", SEED),
                                          **({"query": list(query)} if query else {}),
                                          **({"bounds": sc["bounds"]} if sc.get("bounds") else {})}))
    r = wait_until(lambda: (t := tool("timeline_result"))["done"] and t, timeout=180, every=0.1, what=f"the {sc['name']} timeline")
    if r["result"].get("errors"):
        raise RuntimeError(f"{sc['name']}: inputs failed: {r['result']['errors']}")
    for cls in CLEANUP:
        for path in sorted(set(tool("actors_of_class", class_path=cls)) - before[cls]):
            tool("destroy_actor", actor_path=path)
    return r["result"]["record"]


def granted():
    """Every ability a character Blueprint or an item grants (the dumps' actors and references)."""
    actors = json.loads((DUMPS / "actors.json").read_text())
    refs = json.loads((DUMPS / "references.json").read_text())
    out = {g["abilityClass"]["refPath"].split(".")[0] for a in actors.values() for g in a.get("defaults", {}).get("abilities") or []
           if g.get("abilityClass")}
    out |= {ga for ga, users in refs.items() if ga.split("/")[-1].startswith("GA_") and any("/Items/" in u for u in users)}
    return sorted(out)


def coverage(records):
    started = {e[5] for r in records.values() for e in r["events"] if e[2] == "ability" and e[4] == "activated"}
    played = {f[8] for r in records.values() for rows in r["frames"].values() for f in rows if f[8]}
    montages = json.loads((DUMPS / "montages.json").read_text())["montages"]
    refs = json.loads((DUMPS / "references.json").read_text())
    return [f"never started: {a}" for a in granted() if a not in started] + \
           [f"never played: {m}" for m in sorted(montages) if m not in played and refs.get(m)]


def by_role(events):
    """The events with each frame's grouped by role, each role's in its own order. Inside one frame, the order between
    two characters can be a tie the engine breaks by its own history (two effects applied in one frame expire in one
    frame, in the timer queue's order); a role's own order (the hits, each victim's effects) is kept."""
    return [e for _, e in sorted(enumerate(events), key=lambda p: (p[1][0], p[1][3], p[0]))]


# The tolerance for a recording on another skeleton against the UE4 reference (1.1, approved by the user 2026-11-17):
# actor positions in frames within 5 cm; hit-shape centres, hit contacts, refused projectiles' points, cue and spawn
# locations within 10 cm; everything else exact, rotations, times and the hit's sample included. Where each positional
# field is: a frame row's x, y, z; an event kind's vector (a probe's only for the kinds named here).
TOLERANCE_CM = {"frame": 5.0, "shape": 10.0, "hit": 10.0, "projectile_immune": 10.0, "cue": 10.0, "spawn": 10.0}
VECTOR_AT = {"cue": 8, "spawn": 7, "probe": 7}


def _apart(u, v):
    return sum((p - q) ** 2 for p, q in zip(u, v)) ** 0.5


def _positional(e):
    """An event's tolerance name and its vector's index, or None when the event is exact."""
    kind = e[2] if len(e) > 2 else None
    if kind == "probe":
        return (e[4], VECTOR_AT["probe"]) if e[4] in TOLERANCE_CM else None
    return (kind, VECTOR_AT[kind]) if kind in VECTOR_AT else None


def _same(x, y, worst, tolerant):
    """Whether two event or frame rows agree (frame rows are told by their number, not a kind); worst gathers the
    largest distance per tolerance name."""
    if x == y:
        return True
    if not tolerant or len(x) != len(y):
        return False
    if isinstance(x[2], str):
        where = _positional(x)
        if not where or where != _positional(y):
            return False
        name, i = where
        lo, hi = (x[i], y[i]) if isinstance(x[i], list) and isinstance(y[i], list) and len(x[i]) == len(y[i]) == 3 else (None, None)
        if lo is None or x[:i] + x[i + 1:] != y[:i] + y[i + 1:]:
            return False
    else:
        name, lo, hi = "frame", x[2:5], y[2:5]
        if x[:2] + x[5:] != y[:2] + y[5:]:
            return False
    d = _apart(lo, hi)
    worst[name] = max(worst.get(name, 0.0), d)
    return d <= TOLERANCE_CM[name]


def first_difference(a, b, tolerant=False, worst=None):
    """Where two records of one scenario part: the first frame row or event that differs (events compared by_role).
    tolerant: positions within TOLERANCE_CM count as equal, and worst (a dict) gathers the largest distances."""
    worst = {} if worst is None else worst
    if a.get("roles") != b.get("roles"):
        return f"roles {a.get('roles')} -> {b.get('roles')}"
    ea, eb = by_role(a["events"]), by_role(b["events"])
    for i, (x, y) in enumerate(zip(ea, eb)):
        if not _same(x, y, worst, tolerant):
            return f"event {i}: {x} -> {y}"
    if len(ea) != len(eb):
        return f"{len(ea)} events -> {len(eb)}"
    for role in sorted(set(a["frames"]) | set(b["frames"])):
        fa, fb = a["frames"].get(role, []), b["frames"].get(role, [])
        for x, y in zip(fa, fb):
            if not _same(x, y, worst, tolerant):
                return f"{role} frame {x[0]}: {x} -> {y}"
        if len(fa) != len(fb):
            return f"{role}: {len(fa)} frames -> {len(fb)}"
    return None


def base_attributes(player):
    """The player's attributes at each scenario's start: the abilities suite's, and the run's own Attack and speeds."""
    attributes = player["attributes"]
    return {"MaxHealth": 1000.0, "Health": 1000.0, "Defense": 1000.0, "MaxMana": 100.0, "Mana": 100.0,
            **{a: attributes[a] for a in ("Attack", "AttackSpeed", "Speed")}}


def record_all():
    player = tool("pie_player_state")
    ar.PLAYER_PATH[0] = next(c["path"] for c in tool("list_characters") if c["actor"] == player["actor"])
    base = base_attributes(player)
    records = {}
    for sc in scenarios():
        records[sc["name"]] = run(sc, base)
        print(f"  {sc['name']}: {len(records[sc['name']]['events'])} events, {records[sc['name']]['frame_count']} frames")
    return records


def suite(ctx):
    """Coverage, then each scenario equal to the UE4 reference when there is one."""
    mygame.use(ctx)
    ctx.reset()
    records = record_all()
    (ctx.out / "strict.json").write_text(json.dumps(records, sort_keys=True))
    problems = coverage(records)
    # the committed baseline, exactly; before it existed, the UE4 reference under the tolerance (1.1)
    against, tolerant = (BASELINE, False) if BASELINE.exists() else (REFERENCE, True)
    if against.exists():
        reference = json.loads(against.read_text())
        for name in sorted(set(reference) | set(records)):
            if name not in records or name not in reference:
                problems.append(f"{name}: only in {'the baseline' if name in reference else 'this run'}")
                continue
            worst = {}
            where = first_difference(reference[name], records[name], tolerant=tolerant, worst=worst)
            if worst:
                print(f"  {name}: apart from the reference (cm): " + ", ".join(f"{k} {v:.1f}" for k, v in sorted(worst.items())))
            if where and name in AI_ON:
                print(f"  note {name} (item 4's): {where[:300]}")
            elif where:
                problems.append(f"{name}: {where[:400]}")
    for p in problems:
        print("  " + p)
    return {"ok": not problems, "checks": len(records), "problems": problems, "files": {"strict.json": "Tests/baselines/strict.json"}}
