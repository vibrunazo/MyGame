# MyGame

A roguelike beat 'em up, The Binding of Isaac + Streets of Rage in 3D, migrated from UE 4.25 to 5.8 on branch
`ue58`. Plan, status and decisions are in `MIGRATION.md`.

## Glossary (use these words in code, assets and docs)

| Term | Meaning |
|---|---|
| **Tile** | One cell of a floor's grid (1400 × 2000), holding one streamed tile template |
| **Tile template** | A hand-made map plus its data asset (`UTileTemplate`) that the generator picks from |
| **Room** | What the player experiences: tiles joined with no wall between them, bounded by walls. You enter a room through a door or an opening in a wall. Rooms can span several tiles (and be L-shaped) |
| **Wall / Door** | A room's boundary / a passage through it, which can lock |
| **Closed room** | A one-tile room walled on all sides, whose doors lock until it's cleared (the miniboss reward) |
| **Floor** | One generated layout, start to boss; the flag goes to the next floor |
| **Run** | New game until death |
| **Level** | Only Unreal's maps (`LevelPersistent`, streaming levels), never a floor or a difficulty |

The camera and the darkness work per room, never per tile: no spoiling across walls, but no hiding inside a room either.

## Rules

- C++ never hard-codes asset paths: declare a property and set it on a Blueprint or asset. The cooker can't see paths in code.
- No asset deletions without the user's OK.
- Test with `unreal-agent test`. Package plus `unreal-agent test --on packaged` only for changes that can differ in a cooked build (asset references, config, plugins, startup and menu flow, input mode), or when the user wants an exe.
- The user judges feel (camera, audio) by eye and ear: send videos or clips, not just numbers.

## MyGame's tests and dev tools

How to build, run and test is in the shared section at the end of this file and in `unreal-agent guide`. What only
MyGame knows, and what `guide` doesn't list a command for yet:

- **Editor:** `unreal-agent build`, `start [--render]`, `pie`, `status`, `stop`, from this folder, with no port or
  path. MyGame's port is 18765 (`unreal-agent.toml`, and `Config/UserEditorPerProjectUserSettings.ini` for an editor
  opened by hand). `unreal-agent` is the way to reach the editor; the app's `unreal-mcp` connection is only a
  fallback, and it drops on every editor restart.
- **Tests:** `unreal-agent test` runs every suite in `unreal-agent.toml` (`--only <names>`, `--fast` for layout
  alone), about 2 minutes; results, logs and `summary.json` in `Tests/out/<time>/`. The PIE suites run in a test
  editor of their own on a fixed 1/60 step, so runs repeat exactly, and the baselines describe that step (the dash's two
  airborne frames show there, not on coarser frames). `unreal-agent test --accept <suite>` copies a suite's results
  over its baselines: only for a change meant to alter that behaviour, in a commit of its own. Each PIE suite file has
  `suite(ctx)` (the shared pie-suite contract):

  | Suite | File | Checks | Baseline |
  |---|---|---|---|
  | level | `level_regression.py` | seeds 6032, 101 and 2024 build their recorded level; the doored-room flow | `level_seed_<seed>.json` |
  | abilities | `ability_regression.py` | every player move against a frozen dummy, the boss phases, enemy sight, an enemy attack | `abilities_phase3.json` |
  | loop | `loop_regression.py` | clear a floor, carry over, die, new run, the menu, Game Only input everywhere; Step 0c item 8: a boss's summon holds a doored room's doors until it dies, every HUD icon the same after the floor change, the carried book's move starts there | none (asserts) |
  | camera | `camera_regression.py` | every scenario's camera numbers, played inside the engine; the camera stays still with the player in place; each room's wall cutaway | `camera_room.json` |
  | framerate | `framerate.py` | multi-hit moves land the same hits at 1× and 3× (a 3× frame step) | none (compares) |
  | menu | `menu.py` | from the main menu map, with real keys then a gamepad: start a run, Game Only input and a held W (the stick) moves, pause and resume, back to the menu; runs last, in a rendering editor | none (asserts) |
  | menu-to-run (command; also cooked) | `Source/MyGame/Dev/MenuToRunTest.cpp` | the dev command `MyGame.MenuToRunTest quit`: from the main menu, a real Enter starts a run, which is in Game Only input and a held W moves the player | none (asserts) |
  | dumps | `dumps.py` | Step 0c's content dumps, read in the editor (`dump_content`, `Content/Python/mygame_tools/dump.py`): every ability's and effect's flattened defaults, the character and controller Blueprints, montage timing and notifies, the trees, blackboard and queries, every Blueprint graph as script, referencers, the Blueprint callers of what Step 1 moves, the tag table; about 2 minutes | `dumps/*.json` |
  | strict | `strict.py` | Step 0c's strict recorder (`UCombatRecorder`, `Source/MyGameEditor/Public/CombatRecorder.h`, which gives the record's format): 43 scenarios on fresh spawned enemies, seeded, each recorded frame by frame and event by event inside the engine (transforms, montage sections, abilities, effects in order, tags, attributes, cues, notifies, spawns, and the game's observation points in `Source/MyGame/Dev/TestProbe.h`: hit shapes per sample, hits, effects tried, backslides, shakes, sounds); checks coverage of every granted ability and used montage; about 1 minute | `strict.json`, recorded on the UE5 mannequin after the move (exact); before it, the UE4 reference outside git (`Saved/AssReference/strict.json`) under 1.1's tolerance |
  | cancel | `cancel.py` | Step 0c's cancel matrix: every ability of the player, the enemies and the boss, with a target and without; each frame the recorder asks which granted abilities could start (`CanActivateAbility`, starting nothing), folded by state (running or after, hit confirmed, hit window open or ended, airborne); 40 runs, about 1 minute | `cancel.json`, recorded on the UE5 mannequin after the move; before it, the UE4 reference outside git (`Saved/AssReference/cancel.json`) |
  | ai (manual, `--only ai`) | `ai_behaviour.py` | Step 0c item 4, the AI on the current tree: the boss scenarios (AI on, movement frozen, a real punch across its health marks) compared on the agreed terms (decisions and order exact, an AI-made start within a frame of its cause), and the decision matrix (5 enemy types, 54 situations each: facing, range, token, control; the first decision within a frame, the rest in order); about 3 minutes | the UE4 reference, outside git (`Saved/AssReference/ai.json`) |
  | ai-measures (manual, `--only ai-measures`) | `ai_measures.py` | Step 0c item 4's measures over seeded runs: each enemy type alone and a mixed group of three, against a player standing, turned away, walking or circling, 20 seeds each (480 runs, about 20 minutes); the tolerance rule Step 7 is held to is in the file's header, with its A/A self-test (seeds 1-10 against 11-20) | the UE4 reference, outside git (`Saved/AssReference/ai_measures.json`, with reference videos) |
  | strict-breaks (manual, `--only strict-breaks`) | `strict_breaks.py` | each deliberate break of `Content/Python/mygame_tools/breaks.py` (in memory, never saved) changes its strict scenario, and the scenario repeats after the undo | none (compares) |
  | ai-seed (manual, `--only ai-seed`) | `ai_seed.py` | Step 0c's seeding try: a fresh enemy, both randoms seeded (`seed_random`), repeats frame for frame at 1/60 in three scenarios; another seed differs | none (compares) |
  | smoke (`--on packaged`) | `package_smoke.py` | the cooked game: the menu loads; seed 6032's run builds every room; no missing package; a clean log; every enemy has its AI | none (asserts) |
  | menu-cooked (`--on packaged`) | `menu.py` (`packaged_suite`) | the menu suite's two trips in the cooked game, through its bridge | none (asserts) |

- **Known state:** `Tests/mygame.py` holds MyGame's own test helpers. `reset(ctx)` (named in `unreal-agent.toml`)
  starts seed 6032; `start_run(seed)` starts a run through `UMyGameInstance::StartNewRun` and fails when the floor
  built isn't the seed's baseline floor.
- **Hooks:** the `MyGameTools` toolset in `Content/Python/mygame_tools/toolset.py`, called with `mygame.tool()` after
  `mygame.use(ctx)`.
  `ability_regression.TRACE[0] = True` records every frame of each scenario.
- **Logic tests:** the `layout` suite (`unreal-agent test --only layout`): the `MyGame.Layout.*` automation tests in
  `Source/MyGameEditor/Private/Tests/FloorLayoutTests.cpp`, about 15 s.
- **Video for the user:** `unreal-agent start --render`, `unreal-agent pie`, then `python Tests/camera_video.py
  <out.mp4> [label] [--no-ui]`: a scripted camera demo on seed 6032, recorded by the tools (capture.Recording: a fixed
  1/30 step, every frame taken, real speed; the HUD shows unless --no-ui; no sound, not proof of flicker). For any other
  clip, `unreal-agent video --seconds N`.
- **Packaged build:** `unreal-agent package` (a minute incremental, about 7.5 cold; `[package]` in `unreal-agent.toml`),
  then `unreal-agent test --on packaged` (smoke, menu-to-run and menu-cooked on the cooked game). `Play.bat` runs it.
  `unreal-agent start --packaged` starts it with its bridge (port 18965): `key`, `exec`, `shot`, `status`, then `stop`.
  Development builds carry the bridge (ModelContextProtocol and AgentPlayToolset); Shipping builds don't.
- **Dev commands, launch arguments and debug keys:**

  | Command | Arguments | What it does | Shipping |
  |---|---|---|---|
  | `MyGame.MenuToRunTest` (console) | `[quit]` | From the main menu, a real Enter starts a run; checks Game Only input and that a held W moves the player. Logs `MenuToRun: PASS\|FAIL`; `quit` exits 0 or 1 | absent (`#if !UE_BUILD_SHIPPING`) |
  | `GoToRoom` (console) | `N` | Moves the player into room N of this floor (the room numbers are in the dev overlay) | compiled in; Shipping has no console |
  | `-Seed=` (launch) | `N` | The run's seed (a new run from the menu takes it too) | works |
  | `-Floor=` (launch) | `N` (1-255) | Starts on floor N | works |
  | `-Room=` (launch) | `N` | The first floor starts in room N (once per session) | works |
  | Dev overlay | none | Seed, floor, room, tile and its template, build date, top left | absent |
  | F1 or V (key) | none | Kills every enemy in the room (`BP_MyController`) | no build check found: likely works in Shipping |
  | F5 / F6 (key) | none | Caps the frame rate at 30 / 60 (`BP_MyController`) | no build check found: likely works in Shipping |
  | F3 (key) | none | `stat fps` (`IA_ShowFPS`) | stat commands are compiled out |
  | Test functions | none | `GetTagsForTest`, `SetAttributeBaseForTest`, `InjectTestInput`, `GetInputModeForTest`, `SetUsingGamepadForTest`, `SeedRandomForTest` (the global random and the AI system's stream), `LiftAIBudgetsForTest` (EQS and sight time budgets), `GetAIRandomSeedForTest`, `ResetHitStunCountForTest`, `SeedEveryTickForTest` (a seeded timeline reseeds at every tick's start from the seed and the tick's number); in MyGameEditor, `StartCombatRecord`/`StopCombatRecord` (the strict recorder), `Get`/`SetPropertyTextForTest` and `SetEffectGrantedTagsForTest` (the deliberate breaks): the C++ behind the editor-Python hooks (`set_player_attribute` is the tests' cheat) | editor only (`WITH_EDITOR`) |
- **Replaying a bug:** dev builds show seed, floor, room, tile and build date top left; `-Seed=N -Floor=N -Room=N`
  start there (`Play.bat` passes them through), and the console command `GoToRoom N` moves the player in game.

<!-- unreal-agent-tools workflow: begin (same in every game; edit Docs/GameNotes.md in UnrealAgentTools, never here) -->
## Build and test: the shared workflow

This game is built and tested with UnrealAgentTools (`../UnrealAgentTools`), the same way as every other Unreal game
on this machine. Run `unreal-agent guide` from this folder for the commands, the kinds of test and this game's suites.

Rules:

- Build, run, test, capture and package with `unreal-agent`, wherever `unreal-agent guide` lists a command for it.
  For anything it doesn't list yet, do as this game's notes say and file the gap. Don't commit a launcher, runner,
  wait or capture script to this game. A one-off analysis in the scratchpad is fine.
- `build`, `test` and `package` can take minutes: run them in the background or with a long timeout.
- The user's open editor is fine to work in while they watch; the tools never close it. With it open, `build` compiles
  inside it with Live Coding, and `test`, `run`, `start` and `package` then wait for a full build (they say so). If a
  command names another editor or a game you didn't start, ask the user to close it.
- If the workflow lacks something or could be better, file it: `unreal-agent feedback add "<what, and where>"`. A small
  Python addition any game could use may be made in UnrealAgentTools right away: `unreal-agent guide tools` says how.
  A plugin change, a new command, or a change to the client, launcher or runner is filed instead. It reaches this game
  in a release, with `unreal-agent update`. Never build a private version in this game.
- This game uses the release its `unreal-agent.toml` names. Update when you choose, read what changed, and commit the
  update on its own.
- Only what knows this game lives here: its hooks, dev commands, suites and baselines.
- A baseline changes only with a change meant to alter that behaviour. If another one moves, stop and ask.
- Ship only Shipping builds: a Development package carries the tools' bridge (off unless started with its switch).
<!-- unreal-agent-tools workflow: end -->
