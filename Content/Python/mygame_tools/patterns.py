"""MyGame's enemy patterns (Ass Plan.md Step 7): BT_AIBase written in the plugin's pattern vocabulary, as data assets
(UMyPatternAsset) under /Game/Blueprints/AI/Patterns. Run `build()` in the editor to write them again; the assets are
committed, this script is how they were made.

What each piece stands for in the tree (Ass Docs/Step6.md section 2, Briefs/mygame-step7.md and its decisions):
- Distances are centre to centre (bCentreDistances), as BTDecorator_IsInRange measures. TooFar is 900 for every enemy.
- Passive is the tree's passive branch: its decorators (not in the aggressive range, within 900, and not behind the
  player or the token another's) are its conditions, and it takes over at once (the decorators abort Both). It walks to
  a spot behind the player (EQS_FindAttackSpot as Move To Spot's fields), waits 1 s, then the root's 0.5 s, which no
  condition breaks (shielded). It asks for the token while control is lost (the tree falls into its attack branch).
- Engage, Attack and Approach are the aggressive branch, split by the attack range because the hold is the tactic's: Attack
  holds slot 0 down (BTTask_Attack's key), Approach doesn't (BTTask_StopAttack). Both take the token (TagTarget) and
  chase as the tree's parallel Move To does: beyond the chase range a chase that presses on, within it a wait. The
  chase goes on as they take over from each other (Keeps combo on takeover), as the tree's runs beside its attack task.
- A pause restarts every tactic: the tree aborts its branch when control is lost and runs it again after.
- The phase's entry wait stands for the tree's start (SetRange and the tasks before the first move, a frame each).
"""
import unreal

FOLDER = "/Game/Blueprints/AI/Patterns"
TOO_FAR = 900.0
# EQS_FindAttackSpot (Ass Docs/Step6.md 4.4)
SPOT = dict(SpotBehind=300.0, SpotHalfExtent=500.0, SpotSpacing=100.0, SpotDistanceWeight=1.5, SpotSightScore=1.0,
            SpotScoreRatio=0.75)
LONG = 1.0e4  # a move's time limit: the tree's moves have none
FRAME = 1.0 / 60.0
PASSIVE = 1  # the Passive tactic's index (melee asserts it)
START = FRAME  # the tree's first frame (SetRange): the branches' first tasks (TagTarget asks for the token) come after
LEAD = 3.0 * FRAME  # the attack branch's tasks before its move (TagTarget, ResetFocus, NoWalk)
LEAD_PASSIVE = 2.0 * FRAME  # the passive branch's tasks before its query (ClearTagTarget, StopAttack)
ROOT = 0.5 + 2.0 * FRAME  # the root's Wait 0.5, then the next pass's SetRange and the frame its branch starts in
HOLD_AFTER = 4.0 * FRAME  # the attack task after the attack branch's first tasks and its move

# The tree tasks the AI suite's matrix compares (Q6), by what a command does
QUERY = "Run EQS Query[17]"
SPOT_MOVE = "Move To[21]"
CHASE = "Move To[34]"

# Controller ranges (the controllers' defaults: attack, chase, aggressive), by pattern
KINDS = {
    "PA_Melee": (140.0, 40.0, 200.0),
}


def _text(value):
    """A value as the editor's text (what copy and paste use): structs as (Field=...), arrays as (a,b)."""
    if isinstance(value, bool):
        return "True" if value else "False"
    if isinstance(value, float):
        return f"{value:.6f}"
    if isinstance(value, int):
        return str(value)
    if isinstance(value, _Name):
        return f'"{value}"'
    if isinstance(value, str):
        return value  # an enum value's name
    if isinstance(value, list):
        return "(" + ",".join(_text(v) for v in value) + ")"
    if isinstance(value, dict):
        return "(" + ",".join(f"{k}={_text(v)}" for k, v in value.items()) + ")"
    raise TypeError(value)


class _Name(str):
    """A name or label, quoted in the text."""


def _trigger(**values):
    return dict(values)


class _Builder:
    """The pattern as the plugin's struct fields, by their C++ names."""
    def __init__(self):
        self.commands, self.combos, self.tactics, self.phases, self.names = [], [], [], [], {}

    def command(self, kind, task=None, **values):
        self.commands.append(dict(CommandType=kind, **values))
        if task:
            self.names[len(self.commands) - 1] = task
        return len(self.commands) - 1

    def combo(self, label, commands, **values):
        self.combos.append(dict(Label=_Name(label), CommandIndices=commands, **values))
        return len(self.combos) - 1

    def tactic(self, label, combos, **values):
        self.tactics.append(dict(Label=_Name(label), ComboIndices=combos, **values))
        return len(self.tactics) - 1

    def phase(self, label, tactics, **values):
        self.phases.append(dict(Label=_Name(label), TacticIndices=tactics, **values))
        return len(self.phases) - 1

    def definition(self):
        """FAssAIDefinition as text."""
        return _text(dict(Phases=self.phases, Tactics=self.tactics, ComboSequences=self.combos, Commands=self.commands,
                          Seed=0, bCentreDistances=True, bCapZeroTimeFollowUpsOnly=True))


def melee(attack, chase, aggressive):
    """The base enemies' tree (BT_AIBase with BP_AIController's ranges)."""
    b = _Builder()
    walk = b.command("MoveToSpot", SPOT_MOVE, Duration=LONG, Speed=0.5, bFocusTarget=True, **SPOT)
    # the query, then the Walk and FocusPlayer tasks: three frames before the move
    query = b.command("Wait", QUERY, Duration=3.0 * FRAME, bFocusTarget=True)
    wait_spot = b.command("Wait", Duration=1.0, bFocusTarget=True)
    root_passive = b.command("Wait", Duration=ROOT, bFocusTarget=True)
    root_attack = b.command("Wait", Duration=ROOT, bFocusTarget=False)
    # after an attack pass the key stays down through the root's wait (no condition breaks it) and the next pass's first
    # tasks (only the passive branch's decorators abort them), until the move starts and Approach lets go beside it
    root_held = b.command("Wait", Duration=ROOT, bFocusTarget=False, Shield="FromAll")
    lead_held = b.command("Wait", Duration=LEAD, bFocusTarget=False, Shield="WithExceptions")
    # BT_AttackOrStop's Wait 1.5 after its Attack task: the parallel waits out the running one (a beat from when the
    # attack range was entered, Attack's entry) once its move has ended; it ends as the target leaves the range (its
    # decorators abort it), and the root's wait follows with the key still down
    attack_beat = b.command("Wait", Duration=1.5, bWaitToBeat=True, bEndsWhenConditionsFail=True, bFocusTarget=False)
    lead_passive = b.command("Wait", Duration=LEAD_PASSIVE, bFocusTarget=True)
    press = b.command("Chase", CHASE, Duration=LONG, Speed=1.0, bFocusTarget=False)
    lead = b.command("Wait", Duration=LEAD, bFocusTarget=False)
    stand = b.command("Wait", Duration=0.1, bFocusTarget=False)
    start = b.command("Wait", Duration=START, bFocusTarget=False)

    # Each pass of a branch: the tree's tasks before its move, a frame each, then the move
    root = b.combo("Root wait", [root_passive], bShielded=True)
    root_after_chase = b.combo("Root wait after a chase", [root_attack], bShielded=True)
    # the attack wait is in the aggressive branch, which only the passive branch's decorators abort (the tactic Passive,
    # index 1); the root's wait after it, outside the selector, nothing aborts
    beat_after_chase = b.combo("Attack beat after a chase", [attack_beat], bShielded=True, ShieldExceptions=[PASSIVE],
                               Conditions=[_trigger(MaxRange=attack)], FollowUps=[])
    spot = b.combo("To a spot", [lead_passive, query, walk, wait_spot], FollowUps=[root])
    # the chase ends when the path following says it reached the player (the tree's Move To succeeding), then the root's wait
    chase_on = b.combo("Chase", [lead, press], Conditions=[_trigger(MinRange=chase)], FollowUps=[beat_after_chase, root_after_chase])
    hold_on = b.combo("Close", [stand], Conditions=[_trigger(MaxRange=chase)])
    next_pass = b.combo("Next pass, key still down", [root_held, lead_held, press], ShieldExceptions=[PASSIVE],
                        FollowUps=[beat_after_chase, root_after_chase])
    b.combos[beat_after_chase]["FollowUps"] = [next_pass]
    entry = b.combo("Start", [start], bReactionsMayCut=False)

    # Every condition waits out the tree's first frame (SetRange): until then the pawn is in Start
    after = dict(MinSecondsInPhase=START)
    front = _trigger(MinRange=aggressive, MaxRange=TOO_FAR, MinAngleRelativeToTargetFacing=0.0,
                     MaxAngleRelativeToTargetFacing=60.0, bEitherSide=True, **after)
    taken = _trigger(MinRange=aggressive, MaxRange=TOO_FAR, Token="HeldByAnother", **after)
    starting = b.tactic("Start", [entry])
    passive = b.tactic("Passive", [spot], Conditions=[front, taken], bTakesOverAtOnce=True, bPauseRestarts=True,
                       bAsksTokenWhenPaused=True)
    # Inside the attack range, coming from Start or Passive: the attack branch's pass chases first, and its attack
    # task (the hold) comes a frame after its move, as Attack takes over (BT_AttackOrStop runs beside the move)
    engage = b.tactic("Engage", [chase_on, hold_on, next_pass], Conditions=[_trigger(MaxRange=attack, **after)], FromTactics=[0, 1],
                      bTakesToken=True, bTakesOverAtOnce=True, bKeepsComboOnTakeOver=True, bPauseRestarts=True)
    attack_t = b.tactic("Attack", [chase_on, hold_on, next_pass],
                        Conditions=[_trigger(MaxRange=attack, MinSecondsInTactic=HOLD_AFTER, **after)],
                        bHoldSlot=True, HoldSlot=0, bTakesToken=True, bTakesOverAtOnce=True, bKeepsComboOnTakeOver=True,
                        bPauseRestarts=True)
    # the aggressive branch outside the attack range: inside the aggressive range, beyond 900, or behind the player
    # with the token free or its own
    behind = _trigger(MinRange=aggressive, MaxRange=TOO_FAR, MinAngleRelativeToTargetFacing=60.0,
                      MaxAngleRelativeToTargetFacing=180.0, bEitherSide=True, Token="HeldOrFree", **after)
    approach = b.tactic("Approach", [chase_on, hold_on, next_pass],
                        Conditions=[_trigger(MinRange=attack, MaxRange=aggressive, **after), _trigger(MinRange=TOO_FAR, **after),
                                    behind],
                        bTakesToken=True, bTakesOverAtOnce=True, bKeepsComboOnTakeOver=True, bPauseRestarts=True)
    # their order is the takeover's priority: Engage before Attack, so only Approach hands over to Attack at once
    b.phase("Fight", [starting, passive, engage, attack_t, approach])
    assert (starting, passive) == (0, 1)
    return b


def build():
    """Writes every pattern asset; returns their paths."""
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    out = []
    for name, ranges in KINDS.items():
        b = melee(*ranges)
        path = f"{FOLDER}/{name}"
        asset = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else             tools.create_asset(name, FOLDER, unreal.MyPatternAsset, None)
        # the plugin's fields are edit-defaults-only, which Python can't set on a struct: imported as the editor's text
        if not unreal.MyGameEditorLibrary.set_property_text_for_test(asset, "Definition", b.definition()):
            raise RuntimeError(f"{name}: the definition didn't import")
        asset.set_editor_property("tree_task_names", b.names)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        out.append(path)
    return out
