# MyGame → Unreal Engine 5.8 migration plan

## Status (2026-10-08)

**Phase 1 is done on branch `ue58`.** The project compiles in 5.8. 145 of 148 Blueprints compile; the other three are `ThirdPersonCharacter` (a dead template) and `IconFire`/`IconDefense` (see below). Play-tested headless through the Unreal MCP:
- the level generates
- all rooms stream in
- punches hit, enemies hit back, and knockback works
- clearing rooms kills enemies
- the doored miniboss room drops its reward (`DA_LearnFireball`)
- the boss room's goal is present

Found and fixed along the way, beyond the original plan:
- Native constructors loaded Blueprints through `ConstructorHelpers`, which deadlocks UE5's loader on the `EnemyCharBase` CDO. They now use soft class refs resolved at runtime.
- Character bones didn't refresh when not rendered. Hitboxes are bone-attached, so off-screen enemies, and every character in a headless test, attacked with frozen bones. Characters now use `AlwaysTickPoseAndRefreshBones`.
- UE 5.8 flagged uninitialized struct fields (`FMagnitudePair::Magnitude`, `FLootDrop`, `FAbilityStruct::Input`, `FBuffUI::Color`).

**Play-testing toolset.** `Content/Python/mygame_tools` adds the `MyGameTools` MCP toolset: player state, press ability inputs, teleport, list characters/actors, spawn FX, and set test attributes. It has no arbitrary code or console execution, on purpose.

**Open items from Phase 1:**
- **Cascade → Niagara (done):** `NS_Impact_Player_Weak_small` replaces `P_Impact_Player_Weak_small` in `GetHit_Montage`. The converter output had an unbound sub-UV flipbook playing per DeltaTime; it now uses Direct Index with the original's ease-out frame curve, and side-by-side captures match. Nothing references the Cascade asset anymore. Deleting it, and then disabling the `Cascade` and `CascadeToNiagaraConverter` plugins, is your call.
- **Fire sounds (done):** the three fire cues, which had been silent, now play new Stable Audio sounds. `Fire01Loop_Cue` is an EQ'd seamless loop, `Fire01Once_Cue` picks one of 2 ignites at random, and `FireWhoosh_Cue` picks one of 3 whooshes at random. Sources, processing steps and manifests are in `SourceArt/sfx/fire`. License: Stability AI Community, free under $1M annual revenue (register with Stability before shipping). The cues are rebuilt by `UMyGameAudioEditorLibrary::RebuildSoundCueFromWaves` in the editor-only `MyGameEditor` module.
- **`IconFire`/`IconDefense`:** GE UI data can no longer be a Blueprint class (it's a GE component now). The data survived inside `GE_FireDot`/`GE_StunImmune`. Swap those for native `MyGameplayEffectUIData` components; deleting the two Blueprints afterwards is your call.
- **No mass resave.** Assets that load with problems would lose data: `LevelTest01` (placed enemies fail to load), 4 AdvancedLocomotion animations (bone bindings) and `ThirdPersonCharacter`. Assets get saved as they're edited instead.
- **Fire sounds already silent:** `Fire01Loop_Cue`, `Fire01Once_Cue` and `FireWhoosh_Cue` reference StarterContent audio, which `.gitignore` excludes.
- **Rendering:** room maps bring several directional lights that compete for forward shading. There's no baked lighting (`*_BuiltData` is git-ignored), and the navmesh is generated at runtime (`RuntimeGeneration=Dynamic`).
- **Darker, greyer scene than the 2020 video (fixed):** `SkyLight_1` is now intensity 13 with tint (0.75, 0.83, 1.0), picked against the video. `LightSource2` (the main light) has `ForwardShadingPriority` 1, and `LightSource3` (fill) is no longer an atmosphere sun light, which clears the "multiple directional lights competing" warning. Baked lighting stays out of git: each machine rebakes, and the look doesn't depend on it.
  The cause: baked lighting isn't it. Both directional lights are Movable, and a 2026-10-09 rebuild of all room maps changed nothing visible. The blue look came from the height fog's volumetric fog, which is lit by the Stationary sky light (captured scene, intensity 5). In 4.25 the `AtmosphericFog` actor hazed the sky sphere bright blue, and the sky light captured that. UE5 loads `AtmosphericFog` as a physically based SkyAtmosphere that doesn't touch the sky sphere, so the capture is the sphere's own dark navy (`colors determined by sun position` off, zenith ≈ 0.004/0.013/0.036). The result is a dim, grey volumetric fog. PIE tests of the floor band at RGB mean: now (63,61,70); sky light 15 (≈ video); sky light 13 with tint (0.6,0.72,1.0) is closest; video (45,61,146). Volumetric fog off gives a washed-out cyan.
- **Data quirks:** 3 ability entries use `Input = 200`, which isn't an `EInput` value and now loads as `EInput_MAX`. Enemy montages lack the `ComboStart` section the combo code jumps to. Both predate the migration.
- **Uppercut (fixed 2026-10-09):** since Sep 2020 it's no longer a starting ability, and by Nov 2020 its start-room book was gone. Its only source was `MinibossReward` (Uppercut 1, the other 5 books 0). But `FLootDrop::DropRate` had been ignored since the May 2020 loot refactor, so the pick was random. Now `GetRandomItem` keeps its tiers (items for an empty button first) and picks within a tier by `DropRate`; a tier whose weights are all 0 stays uniform, so tables left at the default behave as before. Test: a fresh player gets Uppercut 300/300 from the miniboss table; with Super+Punch already filled (Smash or Charge Punch), the drop is one of Tat, Kkk or Fireball. **Testing aid, kept on purpose:** a 6th start-room book, `BP_Pickup_Uppercut` at (400, 300), teaches Uppercut. The start-room books stay as a way to debug abilities; decide on them before shipping.
- **Run Punch book titled "None" (fixed 2026-10-09):** book covers take the title and picture from the ability's `AbilityUIdata`, not from the item. `GA_PunchRun` (added Oct 30, 2020) never got its UI data. It now has the name "Run Punch" and the description "Runs and punches", matching `DA_LearnRunPunch`. It still has **no icon**: `Content/tex` has none for it, so its cover and HUD slot show no picture.
- **Unexplained death:** the player died in the boss fight despite a 100k-HP test cheat (the cheat bypasses GAS). Look at this together with the collision bug in Phase 2.

## Phase 2 status (2026-10-09): done

- **Pawn collision stuck off:** fixed.
  - **Cause:** `Tat_Montage` applied `GE_NoPawnBlock` (plus speed and projectile immunity) with a duration of −1, which means forever. Only a broadcast "remove" event ended it.
  - **Fix:** `ANS_ApplyEffect` now owns its effects per character and caps them at the notify's length + 2 s. `AMyCharacter::RefreshPawnCollision` is the only code that sets pawn collision. It waits up to 0.3 s for overlapping characters to separate, then steps out sideways.
  - **Verified:** 36 stress trials (plain, cancel, jump, retrigger) with no stuck collision and no falls through the floor.
- **"Unexplained boss death":** not a game bug. My test cheat zeroed attributes (`GameplayAttributeData(value)` ignores the value); fixed.
- **Conditional effects (Fire Hands → `GE_FireDot`):** this already worked in the original. It matched active effects against the ability's own tags, so `GE_FirePunch` (`activate.punch`) set punches on fire but not kicks. The flaw was narrower: an ability with no tags matched any effect, and `ConditionTag` was unused. A conditional effect now needs an active effect with the condition tag **and** one of the ability's tags. Verified: punch without Fire Hands doesn't burn, punch with Fire Hands burns, kick with Fire Hands doesn't burn. (An intermediate commit, 9054d20, briefly made kicks burn too.)
- **Boss abilities:** they used input `200`, which 5.8 loaded as `EInput_MAX`. It's now `EInput::None = 200`.
- **Abilities:**
  - Combo-cancel detection moved to `PreActivate` (no tag side effects in `CanActivateAbility`).
  - `ComboStart` is only used when the montage has that section.
  - The ability ends once, not once per montage delegate.
  - Ability-system actor info is initialized once.
  - Replaced abilities are cleared from the ability system.
- **Attributes:** Health and Mana are clamped in `PreAttributeChange`; lowering max health or mana lowers the current value.
- **Level walk:** never overwrites a room when both vertical neighbors are taken.
- **GE UI data:** `GE_FireDot` and `GE_StunImmune` use native UI data components. The UI data (icon, name, color) shows correctly in game: the FireDot bar gets the flame icon and StunImmune the shield. `IconFire` and `IconDefense` are kept: nothing gets deleted before full parity with the 4.25 game is proven.
- **Deferred to Phase 4:** `RoomStateRef` (a raw pointer into the grid map; the grid isn't modified after rooms spawn, so it's safe today).
- **Kept:** `ServerTravel` for next-level travel (it works in standalone).

## Phase 3 status (started 2026-10-10)

Parity: you can't recall anything from the 4.25 game that's still missing, so the feature set counts as complete. Deleting anything still needs your explicit OK.

**Regression test:** `Tests/ability_regression.py` (via `Tests/mcp_client.py`). In PIE it freezes every enemy, uses one as a still dummy, and presses the player's buttons through the same path the controller uses. It covers taps and holds of punch and kick, jump + kick and jump + punch, dash, every learned super move, Fireball, a punch after learning, the health-threshold check and an enemy attacking. `diff` compares outcomes: montages, abilities, hit, launch, mana, jump. Three fresh PIE runs gave identical outcomes. The reference is `Tests/baselines/abilities_before_phase3.json`; every step below must reproduce it.

**Steps, each tested against the baseline and committed on its own:**
1. **Done:** native gameplay tags (`MyGameplayTags`, `UE_DEFINE_GAMEPLAY_TAG`) instead of ~55 `RequestGameplayTag` string lookups.
2. **Done:** Enhanced Input. `/Game/Input`: `IMC_Default` plus `IA_Move` (2D), `IA_Punch`, `IA_Kick`, `IA_Cast`, `IA_Jump`, `IA_SuperMod`, `IA_UltraMod`, `IA_Pause` (works while paused) and `IA_ShowFPS`, all with the old keys and gamepad buttons; `Tools/create_input_assets.py` recreates them. `BP_MyController` maps each button to a slot (`AbilityInputs`); its legacy InputAction nodes and every legacy mapping in `DefaultInput.ini` are gone. The F1/V (kill room) and F5/F6 (30/60 FPS) debug keys stay. The regression presses buttons by injecting the real input actions, and now also covers walking and the double-tap dash. Reference: `Tests/baselines/abilities_phase3.json`; three fresh runs gave identical outcomes. Original plan for this step: Keep the slot model (`EInput`): books, loot slot rules, the HUD and the AI's `BTTask_Attack` all use it, so it is the data-driven core, not plumbing. An IMC plus InputActions replace the legacy mappings and the BP controller's input nodes, and the controller maps each InputAction to a slot. Held-button processing stays per frame (as in Lyra's `ProcessAbilityInput`), but moves out of `Character::Tick`. That keeps hold-to-repeat and input buffering.
3. Dash as a double-tap `UInputTrigger` on the move action, firing a gameplay event that `GA_Dash` triggers on. `FAbilityStruct::EventName` goes away (dash is its only user).
4. Air/ground: a `status.airborne` tag and activation tags on the abilities replace the `CanUseOnAir`/`CanUseOnGround` flags. Slot selection keeps "the highest modifier wins, otherwise fall back to plain".
5. Combo window, hit reactions as a HitReact ability plus cues, the hitbox AbilityTask, attribute init and `IncomingDamage`, AIPerception, buff UI data assets: see the table below.

**Found during the baseline (needs your call):** the player's `GA_Smash` carries the boss's triggers (`status.health.75/50/25`). With Smash learned, dropping below 75% health makes the player smash on its own. That's confirmed in PIE (`health_75_with_smash`). It looks copied from `GA_BossSmash1` by the 2020 commit "player ground smash ability". Unless you want it, step 4 removes those triggers from `GA_Smash`.

## Starting point

- UE 4.25 C++ project: one module `MyGame`, about 8.4k lines. 502 assets (186 MB), 39 room maps, each with a `DA_Room*` data asset. Last commit was November 2020.
- UE 5.8 and Visual Studio 2022/2026 are installed. UE 4.25 is not, so the conversion goes straight from 4.25 to 5.8; UE5 still loads 4.x packages.
- What's in C++: the GAS core (ability, attribute set, damage exec), hitboxes, the level builder, the character, the room master and loot.
- What's in Blueprints: input wiring (`BP_MyController` calls `SetAbilityKeyDown`), room spawning (`BP_LevelBuilder` implements `OnBPCreateLevelByName`), AI (BT/EQS), UMG, and all the data: `GA_*`, `GE_*`, `DA_Item*`, `DA_Learn*`.

## How we test

- **Build:** headless UBT, `Engine\Build\BatchFiles\Build.bat MyGameEditor Win64 Development -Project=...`.
- **Unreal MCP:** `unreal-mcp`, `http://127.0.0.1:18765/mcp`, user-scope in Claude Code. It runs while the editor is up, headless included: `UnrealEditor.exe MyGame.uproject -unattended -nosplash -nullrhi`. Useful toolsets:
  - Logs, to read compile and runtime errors
  - AutomationTest, to run tests
  - AbilitySystemInspector, AttributeSet, GameplayTags and GameplayCue, to watch the ASC during PIE
  - PCG, to inspect and generate graphs
  - UMG and Editor
- **Automation tests:** a `MyGameTests` module covering:
  - layout generator: determinism, connectivity, constraints, and 10k seeds with no crash
  - ability functional tests on a test map: a punch hits a dummy, the combo advances, the damage formula holds
  - boot smoke test: every room streams in, the game unfreezes, every RoomMaster registers
- **Screenshots / PIE:** only where a numeric or log check flags something.

## Phase 1: open, compile and run as-is in 5.8

Goal: the old game is playable in 5.8 with the smallest possible changes. This is a commit point.

1. Branch `ue58`. Move `Content/**` (`.uasset`, `.umap`) to Git LFS before any resave; this is decided. Delete `defines.txt` and `editorDebugGame.bat`, which hard-code 4.25 paths. Update `.gitignore`.
2. `.uproject`:
   - Set `EngineAssociation` to `5.8`.
   - Remove `NiagaraExtras` (gone from the engine), `ST_ExportPNG` (a 4.25 marketplace plugin) and `Linter`.
   - Remove the module `AdditionalDependencies`.
   - Enable `EnhancedInput`, `PCG` and `Cascade` (Cascade only until the old FX are converted).
   - `ModelContextProtocol` and `AllToolsets` are already added.
3. Targets: `BuildSettingsVersion.Latest` and `IncludeOrderVersion.Latest`. `Build.cs`: add `EnhancedInput`, `PCG` and `PhysicsCore`.
4. Compile fixes, each confirmed against the 5.8 headers:
   - Anim notify `NotifyBegin`/`NotifyEnd`/`Notify` now take `const FAnimNotifyEventReference&`. This affects every `ANS_*` and `AN_*` class.
   - `UCameraShake` no longer exists; use `UCameraShakeBase` (`MyCharacter.h`, `MyGameInstance.h`).
   - `ASC->AvatarActor` is private; use `GetAvatarActor()` (`DamageExec.cpp`).
   - `UGameplayAbility::AbilityTags` is deprecated; use `GetAssetTags()`.
   - The `NonInstanced` instancing policy is deprecated.
   - `UGameplayEffect::UIData` is deprecated; UI data is now a GE component, read with `FindComponent<UGameplayEffectUIData>()`.
   - `GetAssetsByClass(FName)` became `GetAssetsByClass(FTopLevelAssetPath)` and the include path changed. Phase 4 replaces this call anyway.
   - `IGetHit::IsValidLowLevel()` is a pure virtual that shadows UObject's non-virtual function. Delete it and use `IsValid()`.
   - Clear the warnings from large world coordinates (`FVector` is now double) and the int16 grid math.
5. Content:
   - Open in 5.8 and run the `CompileAllBlueprints` commandlet. Fix broken BP nodes, especially in `BP_MyController`, `BP_LevelBuilder`, the HUD and AI.
   - GE assets upgrade to GE components on load. Resave them.
   - Cascade → Niagara (decided): only one Cascade system exists, `InfinityBladeEffects/.../P_Impact_Player_Weak_small`, played from `GetHit_Montage`. The other 27 effects are already Niagara. Convert it with `CascadeToNiagaraConverter`, then render both versions with the same seed and frames and compare them. Swap the reference only when the Niagara version matches; otherwise fix the emitter by hand. Then drop the Cascade plugin.
6. **Exit check:** PIE on `LevelPersistent` generates rooms, enemies can be punched, a room can be cleared and an item picked up.

## Phase 2: bugs found in the code review

Fix the crashers in Phase 1. Many of the others go away in the Phase 3 and 4 rewrites.

| Where | Bug |
|---|---|
| `MyBlueprintFunctionLibrary.h:58-61` | `FAbilityStruct::operator==` uses `==` statements that do nothing, so it compares only `AbilityClass`. `GetTypeHash` hashes `ID`, which is always 0. As a result, `FindAndRemoveOverlappingAbilities` removes the wrong entries. |
| `MyCharacter.cpp:697,702` | `OnGetHitByEffect` returns `new FActiveGameplayEffectHandle`, leaking memory on every hit. |
| `LevelBuilder.cpp:335,371` | `Settings = &NewSettings` keeps a pointer to a block-scoped local (dangling pointer, undefined behavior). |
| `LevelBuilder.cpp:462` | `FreeCoords[0]` is read without an empty check, so it crashes when the picked tile has no free neighbor. |
| `LevelBuilder.cpp` `BuildGrid` | When both vertical neighbors are taken, the walk steps into an occupied tile and `Grid.Add` overwrites that room. |
| `LevelBuilder.cpp:64` | Time dilation is set to 0 until every room loads. If a room has no data asset, the game stays frozen forever. |
| `LevelBuilder.cpp:742`, `RoomMaster.cpp:55,110` | Null derefs of `Room`, `RoomStateRef` and `RoomType`. There's also a BeginPlay ordering race between the RoomMaster and the LevelBuilder (the "fails on coord 0,0" TODO). |
| `MyGameplayAbility.cpp:53` | The const `CanActivateAbility` adds the `combo.iscancelling` tag. That side effect leaks the tag whenever activation later fails, including on UI and AI checks. |
| `MyGameplayAbility.cpp:427` | Conditional effects ignore `ConditionTag` and query the ability's own tags instead. |
| `MyGameplayAbility.cpp:404` | `break` on a missing effect class drops every effect after it; it should `continue`. |
| `MyGameplayAbility.cpp:275` | A C-style cast to `AMyCharacter` is dereferenced inside `EndAbility`. |
| `MyGameplayAbility.cpp:209` | A C-style cast of `Payload.OptionalObject`. |
| `MyGameplayAbility` | Combo, hitbox and timer state assumes per-actor instancing, but the instancing policy is never set. |
| `MyGameplayAbility.cpp` `OnMontageComplete` | It's bound to Completed, Interrupted, Cancelled and BlendOut, so `EndAbility` runs twice and hitboxes reset on blend-out. |
| `MyCharacter.cpp:400,450` | `InitAbilityActorInfo` runs on every `GiveAbility`. `LearnAbility` removes the old ability from the array but never clears it on the ASC, so replaced abilities stay granted. |
| `ANS_Hitbox.cpp:16` | At runtime, `NewObject` uses the notify (an asset) as its outer. |
| `LootComponent.cpp:167` | It always returns `nullptr`, and a loot entry with a null `Item` crashes. |
| `MyGameInstance.cpp:108` | `ServerTravel` is used in single player; use `OpenLevel`. |
| `MyAttributeSet.cpp` | It clamps only in `PostGameplayEffectExecute` and has no `PreAttributeChange`, so MaxHealth changes don't clamp Health. Stats are set directly in `BeginPlay`. |
| `LevelBuilder::GetGridFromLoc` | `DivideAndRoundNearest` on floats truncated into int16 only works by accident. |
| UObject pointers missing `UPROPERTY` | `LevelBuilder.h:134-135` (`RoomList`, `DoorList`), `HitBox.h:89,100` (`ActorsHit` keys, `OwningAbility`), `MyCharacter.h:263,270`, `MyPlayerController.h:53-68` (widget refs), `BaseController.h:23`. GC can free what these point to. Use `UPROPERTY` + `TObjectPtr`, or `TWeakObjectPtr` for non-owning refs. |
| `RoomMaster.h` `RoomStateRef` | A raw `FRoomState*` into the `Grid` TMap. Any later `Grid.Add` can reallocate the map and leave it dangling. Key rooms by coordinate instead. |

### The "pawn collision stays off" bug (old ability that briefly disables collision)

It's `GA_Tat` (via `Tat_Montage`), plus the Kicker enemy's `KickerKick_Montage`. The chain:
- `ANS_ApplyEffect` NotifyBegin broadcasts `notify.effect.apply`.
- Whichever abilities are listening at that moment apply `GE_NoPawnBlock`, and each stores the handle.
- The GE grants `status.nopawnblock`, and `AMyCharacter::PawnBlockTagChanged` sets the capsule's Pawn channel to Ignore or Block.
- NotifyEnd broadcasts `notify.effect.remove`, and each listening ability removes all of its stored effects.

Why it's fragile:
1. The pairing of begin and end runs through broadcast events to "whatever ability is listening". Combo-cancels and montage blend-outs can deliver the begin and the end to different abilities, or deliver one when nobody is listening.
2. Three writers fight over the same capsule setting: the constructor (`MyCharacter.cpp:115`, defaults to Ignore), `OnMovementModeChanged` (`:982/:991`, which sets Block on landing and Ignore when falling, regardless of the tag) and the tag callback (`:631`). The final state is whichever writer ran last.
3. Turning Pawn blocking back on while two capsules overlap makes character movement push them apart, which can shove a capsule into or under geometry.

**Fix:**
- A single `RefreshPawnCollision()` derives the setting from (tag count, movement mode).
- `ANS_ApplyEffect` applies and removes its own GE, keyed per notify instance (`FAnimNotifyEventReference`), with the notify's duration as a safety cap.
- Re-enabling waits until the capsule no longer overlaps another pawn.
- **First step:** reproduce it in 5.8 while watching tag counts and active GEs through the MCP `AbilitySystemInspector`.

## Phase 3: the data-driven ability system on modern GAS

Keep the design: an ability is a montage list plus effect containers plus anim-notify hitboxes, and items grant effects and abilities. That maps cleanly onto GAS. Replace the plumbing:

| Today | Modern |
|---|---|
| Legacy action mappings, the BP controller, `Tick` polling 8 keys, and the `EInput` enum with +10/+20 modifier math | Enhanced Input: an IMC plus InputActions. An input-config data asset maps InputAction → InputTag, and each granted spec carries its input tag (`GetDynamicSpecSourceTags`). The Super and Ultra modifiers become tags, and the variants select through Required/Blocked activation tags. |
| Double-tap dash computed in `Tick` | A custom `UInputTrigger` for a directional double tap |
| `CanUseOnAir`/`CanUseOnGround` plus Block/Unblock in `OnMovementModeChanged` | A `status.airborne` tag with ActivationRequired/Blocked tags |
| `EventName` abilities (`dash`, `health75`...) | `AbilityTriggers` (GameplayEvent); health thresholds come from the attribute-change delegate |
| Combo tags juggled inside `CanActivateAbility` | A `combo.cancancel` window granted on hit connect or by an `ANS_ComboWindow`, plus `CancelAbilitiesWithTag`. The `ComboStart` section is chosen in `ActivateAbility`. |
| `OnGetHitByEffect` reads asset tags (`data.hitstun`/`knockback`/`launch`/`camshake`/`noapply`) and does the side effects by hand | Sound, particles and camera shake become GameplayCues that carry the hit result. Hitstun, knockback and launch become a passive HitReact ability triggered by `event.hit` with SetByCaller magnitudes. The `data.noapply` hack goes away. |
| Hitbox: `ANS_Hitbox` → event → ability spawns `AHitBox` | Keep the flow, but make it an AbilityTask that owns the overlap shapes. Settings travel as an `FInstancedStruct` payload, not a `NewObject`. Effects are applied with `ApplyGameplayEffectSpecToTarget` and real target data. |
| Stats set in `BeginPlay`, ~40 `RequestGameplayTag(TEXT("..."))` calls | Init from a GE/DataTable, an `IncomingDamage` meta attribute, `PreAttributeChange` clamps, and native tags via `UE_DEFINE_GAMEPLAY_TAG` |
| `UPawnSensingComponent` | AIPerception (sight). Keep the BT/EQS assets; the move to StateTree is a long-term TODO (Phase 7). |
| Buff UI metadata (name, description, color, icon) lived in Blueprint subclasses of the GE UI data (`IconFire`, `IconDefense`); since 5.3 that's a GE component, so Phase 2 copied the values into each effect | A `UBuffUIDataAsset` (primary data asset) per buff as the single source of truth: `DA_BuffUI_Fire`, `DA_BuffUI_StunImmune` with the exact current values. Description becomes `FText`, ready for a future mouse-over tooltip. Each GE's UI component references its data asset instead of holding a copy, and `WBP_DurationBar` reads through it. Verify the flame and shield bars in game. Keep `IconFire`/`IconDefense` until parity is confirmed. |

## Phase 4: the level generator and PCG

**What PCG is for here.** The layout is graph logic: a right-biased random walk, a doored miniboss room roughly every 5 rooms, a treasure branch and a boss at the end. PCG works on spatial points and splines, and C++ needs the room graph anyway for doors, RoomMasters and loot. So the layout stays in C++, rewritten. PCG takes the spatial dressing:
- walls and doors, built with PCG shape grammar (`Subdivide Segment` ships in 5.8) along each room edge, as `[Wall]*` or `[Wall]* Door [Wall]*`
- prop scatter, seeded from the run seed
- optionally, enemy spawn points by difficulty

The steps:

1. A `ULevelLayoutGenerator`, with no world dependency: params plus seed in, an `FLevelLayout` out (`TMap<FIntPoint, FRoomPlan>` plus connections). It picks among free neighbors and backtracks, which fixes the overlap bug. It's covered by automation tests.
2. A room catalogue:
   - Register `URoomDataAsset` as a primary asset type in the Asset Manager, so the assets cook without hard references.
   - Change `LevelAddress` to `TSoftObjectPtr<UWorld>`.
3. C++ streaming with `ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr`, which replaces the BP `OnBPCreateLevelByName`. A loading gate (pause plus a loading widget, with a timeout) replaces time dilation 0.
4. Run state moves into a `URunSubsystem` (UWorldSubsystem). RoomMasters register through their streaming level instead of their world position, which removes the ordering race.
5. A PCG graph builds walls and doors at runtime (Generate On Demand, seeded) from the room bounds and door list. Doors stay real actors because they have logic. The wall cutaway becomes a camera-facing material fade instead of rebuilding HISM heights.
6. Keep the 39 hand-made room maps. The persistent level stays non-World-Partition.

## Phase 5: make the game loop work

1. The loop: main menu → run → rooms → doored miniboss → treasure → boss → next floor (difficulty +1), and death → game over → new run.
2. Keyboard and gamepad both work.
3. Package a Win64 build and smoke-test it.

## Phase 6: the camera

After the migration and the code fixes (Phases 3 to 5). Every tester called the camera the worst part of the game: it moves too much. Your 2020 attempts (May to June: lerped position from the player's place in the room, rotation targets, ease-in-out, wall clamps, offset acceleration) made it worse, not better.

- `ARoomCameraPawn` (`Source/MyGame/Level/RoomCameraPawn.*`, `BP_RoomCameraPawn`) is about 290 lines with ~20 tuning knobs (lerp speed, rotation speed and offset, X/Y ratios, a FoV range, 4 wall clamps, offset acceleration) and two follow functions (`FollowPlayer`, `FollowPlayer2`).
- Plan: first measure what the camera does. Record its position, rotation and FoV per frame during scripted test runs (walking across a room, combat back and forth, dashes, room transitions), and plot how far and how often it moves relative to the player. Then rebuild it from beat 'em up conventions:
  - mostly fixed per room, with a dead zone so small moves and attack lunges don't move it;
  - critically damped smoothing (no overshoot);
  - no rotation during play, or very little;
  - framing that keeps the player and the enemies in view instead of chasing the player.
  Candidates are UE's camera modifiers or a slimmed-down pawn; pick whichever is simpler.
- Compare old and new with the same recorded runs (numbers first, then short clips) and let you pick by feel; play testing decides, not the metrics.

## Phase 7: optional

- **Lighting overhaul (after parity is confirmed):** the current lights are prototypes. Any change moves the look away from the 2020 video, which is still the reference for parity checks, so it waits until parity is done.
  1. **Fully dynamic lighting.** Rooms are assembled at runtime and the main lights are already Movable, so baking adds work (per-machine bakes, re-saved maps) for almost no gain. Set `r.AllowStaticLighting=False` and make the room lights Movable. That removes `*_BuiltData`, Lightmass and the importance volume.
  2. **An explicit mood.** Drive the blue with the height fog color and post-process grading directly. Give the sky light a fixed sky cubemap, or use real-time capture, instead of capturing a sky sphere the camera never sees. Use one sun plus the sky light, and fold the fill light (`LightSource3`) into the sky light. Remove the deprecated `AtmosphericFog`, with your OK.
  3. **Lumen as a high-settings option.** It adds bounce light, light from emissive effects (Fire Hands, pickups) and sky occlusion. It needs a DX12/SM6 GPU and costs several ms per frame. The look must still hold with GI off (`r.DynamicGlobalIlluminationMethod=0`) for low-end PCs and handhelds. Compare captures of the same room both ways, plus GPU frame times, and pick by eye.
- **Generated ability icons (long-term TODO):** render each ability's icon automatically instead of drawing it by hand: a test map plays the ability's montage on the character, a scripted capture writes a render target, and the result becomes the icon texture the book cover and HUD slot use. You planned this in 2020 but never built it. `GA_PunchRun` is the first one that needs it.
- **AI on StateTree (long-term TODO):** move the enemy AI from Behavior Trees (`BT_*`, `BTTask_*`, the blackboards and EQS queries) to StateTree, with the same behavior. Record each enemy type's behavior in scripted PIE runs first (targeting, attack ranges, run-away, the boss summons and buff phases) as the baseline, then port one enemy at a time and compare against it.
- Asset audit, with **no deletions without your OK**. For each test-named asset, list its referencers from the asset registry and say whether a live path reaches it (a map, a character's `Abilities`, a `DA_Learn*`, a loot table). Candidates: ThirdPersonCPP, BSPtest, `GA_Test*` (it may be the real punch), `RT_*`, `ChildActorTest`/`ParentActorTest`, AdvancedLocomotionV4.

## Decisions

1. Git LFS before the resave: **yes**.
2. Cascade → Niagara: **yes**, with visual parity checked (see Phase 1).
3. Deleting assets: **not yet**; do the audit first, then decide one by one.
