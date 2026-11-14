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
- Test in PIE with the regressions (`python Tests/run_all.py`). Package plus `Tests/package_smoke.py` only for changes that can differ in a cooked build (asset references, config, plugins, startup and menu flow, input mode), or when the user wants an exe.
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
  | loop | `loop_regression.py` | clear a floor, carry over, die, new run, the menu, Game Only input everywhere | none (asserts) |
  | camera | `camera_regression.py` | every scenario's camera numbers, played inside the engine; the camera stays still with the player in place; each room's wall cutaway | `camera_room.json` |
  | framerate | `framerate.py` | multi-hit moves land the same hits at 1× and 3× (a 3× frame step) | none (compares) |

- **Known state:** `Tests/mygame.py` holds MyGame's own test helpers. `reset(ctx)` (named in `unreal-agent.toml`)
  starts seed 6032; `start_run(seed)` starts a run through `UMyGameInstance::StartNewRun` and fails when the floor
  built isn't the seed's baseline floor.
- **Hooks:** the `MyGameTools` toolset in `Content/Python/mygame_tools/toolset.py`, called with `mygame.tool()` after
  `mygame.use(ctx)`.
  `ability_regression.TRACE[0] = True` records every frame of each scenario.
- **Logic tests:** the `layout` suite (`unreal-agent test --only layout`): the `MyGame.Layout.*` automation tests in
  `Source/MyGameEditor/Private/Tests/FloorLayoutTests.cpp`, about 15 s.
- **Video for the user:** `unreal-agent start --render`, `unreal-agent pie`, then `python Tests/camera_video.py
  <out.mp4> [label]`: a scripted camera demo on seed 6032, captured
  frame by frame at quarter speed and resampled to 30 fps of game time (no HUD, no sound; not proof of flicker).
- **Packaged build:** `RunUAT BuildCookRun -platform=Win64 -clientconfig=Development -build -cook -stage -pak -iostore
  -archive -archivedirectory=Saved/Packaged` (about 7.5 minutes), then `python Tests/package_smoke.py`. `Play.bat` runs it.
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
- If `build` or `package` names an editor or a game you didn't start, ask the user to close it.
- If the workflow lacks something or could be better, file it: `unreal-agent feedback add "<what, and where>"`. A small
  addition any game could use may be made in UnrealAgentTools right away: `unreal-agent guide tools` says how. It
  reaches this game in a release, with `unreal-agent update`. Anything bigger waits for a session working there.
  Never build a private version in this game.
- This game uses the release its `unreal-agent.toml` names. Update when you choose, read what changed, and commit the
  update on its own.
- Only what knows this game lives here: its hooks, dev commands, suites and baselines.
- A baseline changes only with a change meant to alter that behaviour. If another one moves, stop and ask.
<!-- unreal-agent-tools workflow: end -->
