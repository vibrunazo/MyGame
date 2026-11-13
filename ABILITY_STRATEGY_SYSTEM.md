# Ability Strategy System: design and implementation plan

**Status:** proposal for review, written 2026-11-12. Nothing in it is built yet.

**Home:** this file sits in the MyGame repo for now. It moves to the plugin's own repo (`Docs/Plan.md`) when Step 0 creates it.

The Ability Strategy System is a shared Unreal Engine plugin for the user's action games. It joins two things that live in separate games today: the combat and ability runtime of **MyGame**, and the data-driven ability definitions and pattern AI of **Terra Arcana**. Both games become consumers of it, and so do later games of the same kind.

For reviewers: sections 1 to 3 are the reasoning, section 4 is the resulting design, section 5 the steps, sections 6 and 7 the risks and the decisions still open. Section 2 is the background. Its facts were read from both repos on 2026-11-12 (MyGame branch `ue58` at `3f8d8ef`; Terra Arcana at `c563667` plus the uncommitted work of another session). Facts that were read from files but not confirmed in the editor are marked.

---

## 1. Design goals

1. **One library, several games.** MyGame and Terra Arcana use the same combat and AI code. A third action game can adopt it without copying from either.
2. **Terra Arcana gets MyGame's combat:** combos that cancel the previous animation when timed right, hit windows shaped on bones, hitstun, knockback, knock-ups, hit pause, lunges, and movement abilities that turn body blocking off and back on.
3. **MyGame gets Terra Arcana's data pipeline:** every ability, every enemy's set of abilities and every enemy's behaviour is a data definition that goes to JSON and back with nothing lost. Adding one means adding data, not a Blueprint graph.
4. **Neither game plays differently afterwards.** MyGame keeps its combos, abilities and enemy behaviour. Terra Arcana keeps its abilities, patterns, replays and ratings. Section 1.1 says how "the same" is measured.
5. **Terra Arcana's replay rule holds for all shared code** (section 2.6): the library is replication-correct and gives the same result on a fixed time step at any speed.
6. **Only what is common is shared.** Attributes, damage formulas, input, items, UI, rooms, the server and its tables stay in each game.
7. **Small steps.** After each step both games build, pass their tests and could ship.
8. **Old paths are replaced, not wrapped.** When code moves, the old class is gone: no forwarding classes, no compatibility layers. Assets are different: none is deleted without the user's OK.

Not goals: networked multiplayer play; merging the two attribute sets or damage formulas; visual scripting of abilities; new content beyond one proof in the last step; serving unlike genres.

### 1.1 What "the same" means

| What | Measured by |
|---|---|
| MyGame abilities | `Tests/ability_regression.py`. Its recording (montage and section timeline, damage, tags, push, rise, mana, player movement) must equal the baseline `Tests/baselines/abilities_phase3.json`. The baseline holds 19 scenarios over the 11 player abilities; Step 0 extends it to enemy and boss abilities. |
| MyGame enemies | The Behavior Tree picks its spots without a seed, so two runs never match move for move. Step 0 records behaviour measures over many runs; after the port the same measures must fall inside the same spread. The user also compares before and after videos. |
| MyGame as a whole | `Tests/run_all.py` (level, loop and camera suites). For steps that can differ in a cooked build, a packaged build and `Tests/package_smoke.py`. |
| Terra Arcana | `Terra.AITest all`, `Terra.AbilityTest`, `Terra.MoveTest` and `Terra.ReplayTest` pass. A fixed set of seeded benchmark fights gives the results recorded in Step 0. A stored Lair's JSON loads and saves back unchanged, and so does the server tuning JSON. |

---

## 2. What exists today

### 2.1 Words used here

| Word | Meaning |
|---|---|
| Ability definition | The data that describes one ability (`FAbilityDefinition` in Terra Arcana). |
| Ability template | The asset that holds one definition (`UAbilityTemplate`, a primary data asset). |
| Ability instance | A character's configured copy: a template id plus a level (`FAbilityInstance`). This is what Terra Arcana writes to JSON. |
| Kit | The ordered list of ability instances a character has. The position in the list is the ability's slot. |
| Ability combo | MyGame: one ability that plays a chain of montages, moving to the next when a hit connects. |
| Pattern | Terra Arcana: a character's AI, as data (`FAIDefinition`). It has phases, each with tactics, each choosing among AI combo sequences, each a list of commands. |
| AI combo sequence | Terra Arcana: an ordered list of AI commands (`FAIComboSequence`). Not an ability combo. |
| Hit window | The stretch of an animation during which an attack can hit, and the shapes that hit. |
| Hit payload | What a hit applies: gameplay effects with their magnitudes (damage, hitstun, knockback, launch, camera shake). |
| Titan, raider, Lair | Terra Arcana: the boss the AI plays, the character attacking it (the player or a bot), and the stored fight design they meet in. |
| Room | MyGame: the space bounded by walls that the player fights in. Enemies in a room wake together. |

### 2.2 The two games

| | MyGame | Terra Arcana |
|---|---|---|
| Folder | `D:\docs\Unreal Projects\MyGame` | `D:\docs\Unreal Projects\ue-terra` |
| Repo | GitHub `vibrunazo/MyGame`, branch `ue58` | GitHub `vibrunazo/ue-terra` |
| Engine | Unreal Engine 5.8 | Unreal Engine 5.8 |
| C++ modules | `MyGame`, `MyGameEditor` | `Terra` |
| Game | Roguelike beat 'em up. Single player, nothing replicated. | Boss fights designed as Lairs, stored on a Supabase server, and recorded as replays. |
| Instruction files | `CLAUDE.md`, `MIGRATION.md` | `AGENTS.md`, `Docs/Architecture.md` |
| Tests | Python suites in `Tests/` that drive play-in-editor through UnrealAgentTools, with baselines in `Tests/baselines/` | Console commands: `Terra.AITest rules\|world\|all`, `Terra.AbilityTest`, `Terra.MoveTest`, `Terra.ReplayTest`, `Terra.Benchmark` |
| Combat | Tuned by hand for months with tester feedback. The reference for feel. | A placeholder. The user wants it to feel like MyGame's. |
| Data | An ability is a Blueprint class. Enemy AI is a Behavior Tree. | Abilities and AI are structs. A Lair is JSON. |

Both games use the engine's Gameplay Ability System (GAS). Neither has a `Plugins` folder or git submodules. Another session is active in the Terra Arcana folder, so steps that touch it are scheduled around that work.

### 2.3 MyGame's combat

- **Ability class:** `UMyGameplayAbility` (`Source/MyGame/Abilities/MyGameplayAbility.h/.cpp`, 155 and 424 lines). Each ability is a Blueprint child, `GA_*` (26 assets), and the child's class defaults are the ability's data.
- **Ability combo:** `MontagesToPlay[]` is the chain. A hit that connects advances it, so the ability's next activation plays the next montage. An activation that cancels a hit-confirmed attack starts its montage at the `ComboStart` section. `ComboResetDelay` without a new activation resets the chain. (`HitToComboDelay` is declared but the C++ never reads it; Step 0 confirms whether any graph does.)
- **Cancels:** `TagsIcanCancel`, `bNeedsHitToCancel`, `bCanCancelAfterHitboxEnds`, and the loose tag `combo.cancancel`.
- **Hit windows:** the montage holds `UANS_Hitbox` notify states, each with shapes attached to bones (`FHitboxSettings`), `NumHits` and `HitCooldown`. `UAbilityTask_HitboxWindows` samples the windows on animation time, between frames too, and spawns an `AHitBox` (`Source/MyGame/Player/HitBox.cpp`, 441 lines).
- **Hit payload:** `EffectsToApply[]`, each an effect class plus SetByCaller magnitudes (`data.damage`, `data.hitstun`, `data.knockback`, `data.launch.x/y/z`, `data.camshake`). Also `EffectsToApplyToSelf[]` (on from start to end) and `ConditionalEffects[]` (applied only while the user has an effect with a given tag).
- **Hit reactions:** `UHitReactionEffectComponent` on a gameplay effect makes it stun, knock back, launch or shake the camera, at the strength in the magnitudes.
- **Other ability data:** `HitPause`, `MaxHits` and `bSkipToRecovery` (jump to the `Recovery` section), `TagThatDeactivateMe`, `RootMotionScale`, `MontagesSpeed`, facing and target-lock flags, `bStartsCombat`, hit and block sounds and particles, UI data. Cost and cooldown are the engine's (`CommitAbility`, effect classes in the class defaults), and so are the tag lists (for example `activate.groundattack`, `activate.flyingattack`).
- **On the character:** `AMyCharacter` (`Source/MyGame/Player/MyCharacter.cpp`, 1,469 lines) holds about 500 lines of combat mechanics: knockback and launch, hitstun count and stun immunity, hit pause (`CustomTimeDilation`), root-motion speed curves, body-block switching (`RefreshPawnCollision`, `StepOutOf`), the airborne tag, and the control checks `HasControl` and `HasMoveControl`. The ability refers to `AMyCharacter` 12 times, and hits go through the `IGetHit` interface.
- **Projectiles:** `UCastAbility`, `ICastProjectile`, `AMyProjectile`.
- **Which abilities are pure data:** 19 of the 26 `GA_*` Blueprints add no graph logic. The seven with graphs are `GA_Cast`, `GA_Dash`, `GA_SelfEffect`, `GA_Summon`, `GA_BossSelfDebuff`, `GA_Test` and `GA_Test2`. *(Found by listing node types in the asset files; not confirmed in the editor.)*
- **Stays in MyGame:** `UMyAttributeSet`, `UDamageExec`, input (`EInput`), loot, UI.

### 2.4 MyGame's enemy AI

One Behavior Tree, `BT_AIBase` (with `BT_AttackOrStop`, the blackboard `BB_AIBase` and the query `EQS_FindAttackSpot`), runs every enemy and the boss. The controllers (`BP_AIController`, `BP_AIContKicker`, `BP_AIContWizard`, `BP_AIContBoss`, all on C++ `AMyAIController`) differ only in ranges and in the boss's event responses. *(The tree's exact shape was read from asset names and the C++ it calls, not in the editor. Step 0 reads it properly.)*

- An enemy is idle until it sees the player (AIPerception sight), then its whole room wakes.
- While passive it walks to a spot behind the player, chosen by the query (a grid round the player, scored by being behind them and in sight, one of the best quarter at random), and keeps facing the player.
- It attacks when the player has their back to it, or when it may take the player's attack token (`AggroList` on the player: the list is empty or already names this enemy), and it isn't too far. Then it chases and holds the attack button, which is what makes ability combos chain.
- It doesn't act while stunned, launched or dead.
- The wizard also runs away (`BTTask_FindLocationToRun`).
- The boss answers the gameplay events `status.health.75`, `.50` and `.25` with one ability each (`AMyAIController::EventResponses`). Each fires once and isn't retried if it can't start.

### 2.5 Terra Arcana's abilities

- `FAbilityDefinition` (`Source/Terra/Public/Data/TerraArcanaTypes.h`): one montage, `CastTime` and `RecoveryTime` (the montage is stretched to fit them), `SelfEffects[]` (`FTimedEffect`: applied and removed on montage events such as `Anim.Start` and `Anim.Hit`, with an offset, or toggled), `Projectiles[]` (`FProjectileDefinition`: magnitude, effects, speed, lifespan, radius, angle), `Dashes[]` (`FDashDefinition`: distance, duration, angle), `ManaCost`, `Cooldown`, `PowerStat`, `bIsAttack`, and `GetReach()`.
- A melee hit is a projectile that doesn't travel: a 100 cm sphere spawned at `Anim.Hit`.
- There are no ability combos, cancels, knockback, launch, hit pause, bone hit windows or summons.
- The runtime is C++ `UTerraAbility` plus the Blueprint `GA_AbilityBase`, whose graph holds the montage, projectile and dash logic. One class runs every ability: the definition is set on the granted ability (`SetAbilityInstance`).
- The same data is read by the bot (`TerraBotController`), the damage calculation, the build compiler (`UTerraBuildLibrary`) and the server tuning table (`UTerraRewardTuning::ToTuningJson`), which derives each archetype's best damage rate from the projectiles' magnitudes and the ability's cycle time.

### 2.6 Terra Arcana's pattern AI and its rules

- `FAIDefinition`: `Phases`, `Tactics`, `ComboSequences` and `Commands`, linked by index, plus a `Seed`.
- Commands (`FAICommand`): `UseAbility`, `MoveToRange`, `Strafe`, `Wait`, `FaceTarget`, `Chase`.
- Conditions (`FAITrigger`): range, angle to the target, angle relative to the target's facing, own and target health, line of sight, time in phase, tactic and fight, the target's motion, minions alive and defeated, tag counts.
- `FTerraPatternRunner` (`Source/Terra/Public/AI/TerraPatternRunner.h`) is a plain struct with no world. Each tick it is given a snapshot (`FTerraPatternSnapshot`) and returns actions (`FTerraPatternAction`). Its random draws are seeded, so a fight repeats. `AIRevision` (now 2) is part of the game's combat version.
- `UTerraPatternComponent` on the AI controller builds the snapshot and carries the actions out. `TerraPatternRules` and `TerraPatternCheck` validate patterns. `Terra.AITest rules` steps the runner without a world; `Terra.AITest world` runs patterns in the arena.
- **The replay rule** (`AGENTS.md`): fights are recorded with the engine's replay system, so game logic runs only on the authority, and everything a viewer sees is replicated state, a replicated montage or a gameplay cue, never a local-only effect. `UTerraMatchSubsystem::ReplayVersion` is bumped when replicated classes change.
- Benchmark fights run in hidden worlds on a fixed 1/30 s step, far faster than real time. Code that depends on wall-clock time or frame rate breaks them.
- A Lair's definition is stored as JSON (`FJsonObjectConverter`, `Data/TerraSerializationLibrary`) in the server's `lairs` table. The JSON holds property names and enum value names, so renaming either breaks stored rows.

### 2.7 Side by side

| Capability | MyGame | Terra Arcana |
|---|---|---|
| An ability is defined by | Blueprint class defaults | A struct in a data asset |
| Montages per ability | A chain | One |
| Timing | As animated, times a speed | Montage stretched to data times |
| Melee hit | Shapes on bones during notify windows | A sphere at one moment |
| Cancels on hit confirm | Yes | No |
| Hitstun, knockback, launch | Yes | Poise stagger only |
| Hit pause, root-motion scaling, body-block switching | Yes | No |
| Projectiles, dashes | Yes (C++ and a Blueprint graph) | Yes (data lists) |
| Timed and toggled self effects | Start to end only | Yes |
| Conditional effects, summons | Yes | No |
| Ability levels | No | Yes |
| Enemy AI | Behavior Tree and a query, unseeded | Data pattern, seeded, testable without a world |
| Boss phases | One-shot event responses | Phases with conditions and entry sequences |
| Attack tokens, spot behind the target | Yes | No |
| Replication-correct | No | Yes, by rule |
| JSON | No | Yes |

### 2.8 How code is shared today

`D:\docs\Unreal Projects\UnrealAgentTools` is a git repo with two halves, and the games use them differently:

- **A Python package** (`unreal_agent_tools`: a client for the editor's MCP server, and an editor launcher), installed once on the machine with `pip install -e`. **Both games use it.** It needs nothing in a `.uproject` beyond the engine's own `ModelContextProtocol` and `AllToolsets` plugins. MyGame's `Tests/harness.py` imports it and reaches the game through MyGame's own Python toolset (`Content/Python/mygame_tools`).
- **One C++ editor plugin**, `Plugins/AgentPlayToolset` (console commands, key presses and clicks inside play-in-editor), added on 2026-10-31. **Only Terra Arcana loads it**, with `"AdditionalPluginDirectories": ["../UnrealAgentTools/Plugins"]` in its `.uproject`. MyGame's `.uproject` has no `AdditionalPluginDirectories` entry and its logs never mention the plugin.

So no C++ plugin is built by both projects yet. This plan does not load its plugin that way (3.2): a path to a sibling folder isn't part of a game's repo, so a clone of the game on another computer wouldn't build.

What Terra Arcana's build shows about such a plugin (read from its folders and log, UE 5.8): the engine mounts it as an "External plugin". The compiled module goes to the **game's** `Binaries/Win64` and the object files to the game's `Intermediate/External`. Only the generated headers are written into the plugin's own `Intermediate`. Two games therefore don't share compiled binaries. (UnrealAgentTools' `CLAUDE.md` says the plugin's own `Binaries`; that folder doesn't exist.)

### 2.9 Rules that bind this work

- **MyGame:** C++ never hard-codes asset paths. No asset is deleted without the user's OK. Test in play-in-editor with the regressions; package and run the smoke test only for changes that can differ in a cooked build. Feel is judged by eye and ear, so send videos. Abilities are started by input or by the AI, never by themselves.
- **Terra Arcana:** the replay rule above. No secret keys in a build. Ask before anything destructive on the server.
- **Both:** control the editor through UnrealAgentTools. One editor per port, so a second project's editor needs its own. Token cost matters more than speed.

---

## 3. Potential solutions, and the ones picked

Each question lists the options considered, then the pick and the reason.

### 3.1 Overall approach

| Option | For | Against |
|---|---|---|
| A. Keep two systems, copy ideas by hand | Nothing to move now | Every mechanic is built and tuned twice. Terra Arcana's combat becomes a rewrite of MyGame's from reading its code. |
| B. Put Terra Arcana's systems into MyGame as they are | MyGame is data-driven at once | MyGame loses combos, cancels, reactions and its tuning. |
| C. Put MyGame's systems into Terra Arcana as they are | Terra Arcana gets the combat | Terra Arcana loses data definitions, the seeded AI and replay-correctness. |
| D. One library by extraction: runtime from MyGame, data model and AI from Terra Arcana, joined at the ability definition | Each half already exists, tuned and tested | Both games are refactored. |
| E. Rebuild both on engine frameworks (StateTree, Lyra-style ability sets) | Engine-maintained | Neither is JSON-friendly or seeded. Two working systems and their tests are thrown away. |

**Picked: D.** Each half is already working in one game with tests around it, and extraction keeps those tests as the gates.

### 3.2 Where the library lives and how a game loads it

| Option | For | Against |
|---|---|---|
| A. Precompiled and installed in the engine's plugin folder, as Fab plugins are | No project setup. The form to hand to other people. | A repackage and reinstall for every code change. Tied to one engine version. A game's repo doesn't record the version it needs. |
| B. A copy in each game | Simple | The copies drift. |
| C. Own repo, a git submodule under each game's `Plugins` | A clone of a game brings the plugin, at the commit that game was tested with. An ordinary project plugin, in the editor and in a packaged build. A change made for one game reaches the other only when it pulls. | Two working copies on one machine, synced through a remote. |
| D. Own repo in a sibling folder, mounted with `AdditionalPluginDirectories` | One working copy. The way Terra Arcana loads `AgentPlayToolset` (2.8). | A clone of a game alone doesn't build. No per-game pin. A change reaches both games at once. |
| E. Inside UnrealAgentTools' `Plugins` folder | Terra Arcana already mounts it | Shipped gameplay code in an editor-tools repo. The same clone problem as D. |

**Picked: C** (the user's decision, 2026-11-13, after first asking for D): the games must build from a clone on another computer, and the plugin should be a regular one that could be given to other people.

```
github.com/vibrunazo/AbilityStrategySystem            its own repo; the plugin is the repo's root
    AbilityStrategySystem.uplugin
    Source\AssPattern, AssCombat, AssDriver, AssEditor
    README.md, CLAUDE.md, Docs\Plan.md

MyGame\Plugins\AbilityStrategySystem\                  submodule
ue-terra\Plugins\AbilityStrategySystem\                submodule
```

Each game adds the submodule and enables the plugin in its `.uproject`. The submodule's URL is written relative (`../AbilityStrategySystem.git`), so it follows wherever the game's own remote is. A game is cloned with `git clone --recursive`.

**Day to day.** There are two working copies on the development machine, one inside each game, and no third. A plugin change is made inside the game whose step needs it, tested through that game, then committed and pushed from the submodule folder; the game's repo then commits the new submodule commit. The other game takes the change when it updates its submodule and passes its own gates. Each game compiles the plugin into its own `Binaries`, which are build output and never committed.

**For other people later.** `RunUAT BuildPlugin` on the same repo makes the precompiled form of option A. Running it now and then also proves the plugin compiles with no game around it.

### 3.3 One plugin or two, and its modules

| Option | For | Against |
|---|---|---|
| A. Two plugins, combat and AI | Each can be versioned alone | Two descriptors and a compatibility matrix, for halves that are always used together. |
| B. One plugin, one module | Simplest | The AI's decision code would link against the engine's world and ability classes, and could no longer be tested without a world. |
| C. One plugin, modules along the real seams | One version; the seam stays visible | A few more build files. |

**Picked: C.** Module names carry the plugin's acronym, `Ass`, as a prefix (the user's naming rule, 2026-11-12). So do the plugin's types (4.1). No engine module has any of these four names.

| Module | Holds | Depends on | Comes from |
|---|---|---|---|
| `AssPattern` | The AI data (`FAssAIDefinition` and its parts), the runner, the rules and checks, and the rule tests. No world. | Core, CoreUObject, GameplayTags | Terra Arcana |
| `AssCombat` | The ability definition and template, the ability class, hit windows, the hitbox, hit reactions, the combat component, the impact cue | Engine, GameplayAbilities, GameplayTasks, GameplayTags, Niagara | MyGame's runtime, Terra Arcana's definition |
| `AssDriver` | What runs a pattern on a pawn: builds the snapshot from the world, carries out the actions, grants attack tokens, searches for spots | The two above, AIModule, NavigationSystem | Terra Arcana's component, MyGame's tree tasks |
| `AssEditor` (editor only) | Converters, validation, values baked from montages | The above, UnrealEd | New |

The plugin holds code only (`CanContainContent: false`). Every asset comes from the game through a property or a project setting.

### 3.4 The ability data model

| Option | For | Against |
|---|---|---|
| A. MyGame's: a Blueprint class per ability, plus JSON export of its defaults | No conversion | An ability is still a class. Terra Arcana's bot, compiler and tuning table would read class defaults. |
| B. Terra Arcana's struct as it is, with a MyGame subclass for the rest | Small | Two models forever. |
| C. One struct that is a superset: Terra Arcana's definition gains MyGame's fields as fixed, typed lists | Terra Arcana's readers keep working. JSON, diffing and validation stay simple. | The struct grows; new mechanics need new fields. |
| D. An open list of polymorphic actions, or a graph | Most flexible | Hard to serialize, validate and diff, and for the server table and the bot to read. Neither game needs it. |

**Picked: C.** 19 of MyGame's 26 abilities are already pure data, and Terra Arcana's tools already read this struct. A fixed vocabulary is also what a server can check. Defaults are chosen so that every existing Terra Arcana template means what it means today. The field map is in section 4.2.

### 3.5 How a definition becomes a running ability

| Option | For | Against |
|---|---|---|
| A. A Blueprint class per ability that reads its own definition | Smallest change in MyGame | Adding an ability still means adding a class. |
| B. One generic C++ ability class; the definition arrives with the granted ability | Adding an ability is adding data. Terra Arcana works this way already. | GAS reads tags from the class, not from an instance (see below). |
| C. As B, and a template may name a subclass | An escape hatch for logic data can't express | Can be overused. |

**Picked: B, with C as the escape hatch.** The engine reads an ability's tags from its class: `UAbilitySystemComponent::CancelAbilities`, for one, tests `Spec.Ability->GetAssetTags()`. So the plugin's ability class applies the definition's tags itself: activation checks, tags owned while active, and cancelling. Cost and cooldown come from the definition through the engine's virtual getters. This is the largest parity risk on the MyGame side, so it gets its own step and gate. Terra Arcana's `GA_AbilityBase` graph is ported to C++ in the plugin.

### 3.6 Where hit timing lives

| Option | For | Against |
|---|---|---|
| A. In data (Terra Arcana's `CastTime`) | The server and the bot can read it | Tuning a hit against the animation by number, not by eye. |
| B. In the montage (MyGame's notify windows) | Tuned by eye, frame against frame | The server and the bot can't open a montage. |
| C. Both: the montage says when and where, the definition says what, and derived numbers are baked | Keeps MyGame's tuning and Terra Arcana's readers | A bake step that must not go stale. |

**Picked: C.** The montage's notify windows give the time and the shapes. The definition gives the hit payload. A definition may instead stretch its montage to `CastTime` and `RecoveryTime`, which is what every existing Terra Arcana template does. The numbers other code needs (reach, time to first hit, total length) are baked from the montage into the template by an editor check, and a test fails when they are stale.

### 3.7 How combat attaches to a character

| Option | For | Against |
|---|---|---|
| A. A shared base character class | Least wiring | Forces one class tree on every game. |
| B. A component on any `ACharacter` that has an ability system, plus a small interface for what only the game knows | Works with both games' characters as they are | The ~500 lines in `AMyCharacter` must be untangled. |
| C. In an ability system component subclass | One place | Movement and collision code doesn't belong there. |

**Picked: B.**

### 3.8 Enemy AI

| Option | For | Against |
|---|---|---|
| A. Keep MyGame's Behavior Tree, share only the code that carries out actions | No port | MyGame's enemies aren't data. Goal 3 fails. |
| B. MyGame's enemies on the pattern system, extended with what they need | Data, seeded, testable without a world | Behaviour must be re-created and compared statistically. |
| C. Both games on StateTree | Engine-maintained | Not JSON, no seeded stepping without a world. Terra Arcana's tests would be rewritten. |
| D. Generate Behavior Tree assets from data | Reuses the tree | Indirect, and still unseeded. |

**Picked: B.** It replaces the "AI on StateTree" item in `MIGRATION.md` Phase 7. What the pattern system must gain is in section 4.3.

### 3.9 When combat becomes replication-correct

| Option | For | Against |
|---|---|---|
| A. While extracting it | One pass | A move and a behaviour change in the same step; a failure can't be attributed. |
| B. As its own step after extraction, before Terra Arcana uses the combat module | Each step has one cause of failure | One more step. |
| C. Never; Terra Arcana forks the code | No work now | Goal 1 fails. |

**Picked: B.**

### 3.10 What data may name, and what crosses to the server

| Option | For | Against |
|---|---|---|
| A. Asset paths anywhere, including JSON from the server | Fully open | A server row could make the game load any asset. |
| B. Ids only, everywhere | Tight | Templates in the editor would lose asset pickers. |
| C. Templates hold asset references; only instances and ids travel | Terra Arcana's rule today | None found. |

**Picked: C.** A template is a local asset with soft references. What travels between game and server is an instance (template id and level), a pattern that names abilities by slot, and effects named by id from a list the game owns. Full definitions also go to JSON and back, for tools, tests and authoring, but a game doesn't accept a full definition from a server.

### 3.11 Gameplay tags and attributes

**Tags.** Option A: each game keeps its own tag names and tells the plugin which is which in settings. Option B: the plugin declares one set of native tags and each game adds tag redirects in its config. **Picked: B.** The code keeps using native tag variables, and redirects need no asset to be resaved. MyGame's names are kept for combat (`status.hitstun`, `data.knockback`, `notify.hit.start`, `combo.cancancel`) and Terra Arcana's for montage events (`Anim.Start`, `Anim.Hit`). Where both games name the same thing differently, the plugin keeps one and the other game redirects.

**Attributes.** The plugin owns no attribute set. What it must read (attack speed, haste, mana) is named in project settings as attributes. Damage calculations stay in each game.

### 3.12 Versions

| Option | For | Against |
|---|---|---|
| A. Nothing beyond the submodule | Simple | Nothing tells a game that a newer plugin commit would make its stored data play differently. |
| B. Behaviour revision numbers in the plugin, part of each game's combat version | Terra Arcana already does this with `AIRevision` | Relies on bumping them. |
| C. Each game records the plugin commit it last passed its gates against | Shows when a game is running unverified plugin code | The submodule already is this record. |

**Picked: B.** The plugin has `AIRevision` and a new `CombatRevision`, bumped when existing data would play differently. The commit a game was verified against is its submodule commit: a game moves its submodule forward only in a commit that passed its gates.

---

## 4. The resulting design

### 4.1 Names

The plugin is named `AbilityStrategySystem`. Its acronym, `Ass`, prefixes every module (3.3) and every type the plugin declares, after Unreal's own type letter. This is the user's rule (2026-11-12). It is written `Ass`, not `ASS`, by the user's choice (the engine itself would use capitals, as in `UPCGComponent`).

The prefix also removes a sequencing problem. Unreal's header tool rejects two reflected types with the same name in one build ("shares engine name"), whatever module each is in. With the prefix, no plugin type has the name of a type a game still declares, so a game can keep its old type while the plugin's replacement is brought in.

| Today | In the plugin |
|---|---|
| Terra Arcana's data types: `FAbilityDefinition`, `FAbilityInstance`, `UAbilityTemplate`, `FProjectileDefinition`, `FDashDefinition`, `FTimedEffect`, `FAIDefinition`, `FAIPhase`, `FTactic`, `FAIComboSequence`, `FAICommand`, `FAITrigger`, and their enums | The same names with the prefix: `FAssAbilityDefinition`, `FAssAbilityInstance`, `UAssAbilityTemplate`, `FAssAICommand`, `EAssAICommandType`, and so on |
| `FTerraPatternRunner`, `FTerraPatternSnapshot`, `FTerraPatternAction`, `UTerraPatternComponent` | `FAssPatternRunner`, `FAssPatternSnapshot`, `FAssPatternAction`, `UAssPatternComponent` |
| `UMyGameplayAbility` | `UAssAbility` |
| The combat part of `IGetHit` | `IAssCombatant` |
| `UANS_Hitbox`, `FHitboxSettings`, `UAbilityTask_HitboxWindows`, `AHitBox` | `UAssHitboxNotifyState`, `FAssHitboxSettings`, `UAssHitboxWindowsTask`, `AAssHitBox` |
| `UHitReactionEffectComponent`, `FEffectContainer` | `UAssHitReactionEffectComponent`, `FAssEffectContainer` |
| New | `UAssCombatComponent`, `UAssAttackTokenComponent` |

What a rename must not touch:

- **Serialized names.** No property and no enum value is renamed. `FJsonObjectConverter` writes those and not type names, so stored Lairs and the tuning table read as before. Step 5's gate checks it on a stored Lair.
- **The primary asset type of ability templates** stays `AbilityTemplate`. Stored ability ids carry it (`AbilityTemplate:<name>`). Terra Arcana's template class returns it by literal today (`AbilityTemplate.h:41`), and the renamed class keeps doing so.
- **Gameplay tag names** (3.11). They are data in assets and montages, not code names, and take no prefix. The C++ tag variables sit in an `AssTags` namespace.

Moved and renamed types get class, struct and enum redirects in each game's `DefaultEngine.ini`.

### 4.2 The shared ability definition

| In the shared definition | From MyGame | From Terra Arcana | When the other game doesn't set it |
|---|---|---|---|
| Steps, one montage each | `MontagesToPlay[]` | `AbilityMontage` | One step |
| Speed, or fit to times | `MontagesSpeed` | `CastTime`, `RecoveryTime` | MyGame: as animated. Terra Arcana: fitted. |
| Chain rules | `bCanCombo`, `ComboResetDelay`, the `ComboStart` section | none | No chain |
| Cancel rules | `TagsIcanCancel`, `bNeedsHitToCancel`, `bCanCancelAfterHitboxEnds` | none | Cancels nothing |
| Hit payload | `EffectsToApply[]` | Projectile `EffectClasses`, `Magnitude` | |
| Hit windows | `UANS_Hitbox` in the montage | none | Sphere projectile at `Anim.Hit` |
| Early end | `MaxHits`, `bSkipToRecovery`, `TagThatDeactivateMe` | none | Off |
| Hit pause, root-motion scale | `HitPause`, `RootMotionScale` | none | 0 and 1 |
| Facing and target lock | The five flags and `TypesToTestTargetLock` | none | Off |
| Self effects | `EffectsToApplyToSelf[]` | `SelfEffects[]` | MyGame's become "from start to end" entries |
| Conditional effects | `ConditionalEffects[]` | none | None |
| Projectiles | `UCastAbility` and its notify | `Projectiles[]` | None |
| Dashes | `GA_Dash`'s graph | `Dashes[]` | None |
| Summons (new list) | `GA_Summon`'s graph | none | None |
| Cost and cooldown | Effect classes in class defaults | `ManaCost`, `Cooldown` | Either form |
| Tags | Class defaults | `bIsAttack` | |
| Scaling stat | Through magnitudes | `PowerStat` | A key the game's damage rules read |
| Presentation | Sounds, particles, UI data | Projectile travel and impact effects | |
| Baked values (new) | none | `GetReach()` | Reach, time to first hit, length |

The plugin's ability class runs a definition as a fixed set of actions fired by the montage: hit windows, projectiles, dashes, timed self effects, summons. Each game still supplies its own projectile actor through a small interface.

### 4.3 What the pattern system gains for MyGame's enemies

| MyGame behaviour | Today | In the pattern system |
|---|---|---|
| Idle until the player is seen, then the room wakes | AIPerception, `OnPawnSeen` | Stays in MyGame, which starts the pattern. |
| Walk to a spot behind the player | `EQS_FindAttackSpot` | New move command: take a spot round the target, by range band and arc relative to its facing, in sight, picked with the pattern's seeded draw. The search runs in the driver module. |
| Attack when the player's back is turned | `BTDecorator_IsBehindEnemy` | Exists: the angle relative to the target's facing. |
| Only some enemies attack at once | The player's `AggroList` and three tree nodes | New: attack tokens on the target, granted in a fixed order, as a condition and as something a tactic takes and gives back. |
| Attack and chase ranges | Controller defaults | Exists: range conditions. |
| Hold the button so the ability combo chains | `BTTask_Attack` | `UseAbility` gains a hold. |
| No acting while stunned, launched or dead | Three decorators | The snapshot has `bStaggered`; it gains "can act" and "can move" from the combat component. |
| The wizard runs away | `BTTask_FindLocationToRun` | `MoveToRange` if it backs off far enough, else a retreat command. To settle when the tree is read. |
| Boss moves at 75, 50 and 25% health | One-shot event responses | Exists: phases with health conditions and entry sequences. |
| Boss summons | An ability | The ability's summons list. Conditions on minions exist. |
| Crowd avoidance | `UCrowdFollowingComponent` | Stays on MyGame's controller. The driver works with any AI controller. |

Every new field defaults to off, so existing Terra Arcana patterns run as before.

### 4.4 What stays in each game

- **MyGame:** attributes and `UDamageExec`, input and button slots, sight and room waking, loot, UI, its projectile actor, its AI controller class.
- **Terra Arcana:** attributes and its damage calculation, builds and levels, mana and per-slot cooldown rules, poise, the fight log, the bot, the server tuning, Lair serialization, its AI controller, its world tests.

---

## 5. Suggested implementation steps

Each step is one session in one game's folder, working in that game's copy of the plugin. Only one session edits the plugin at a time, and each step ends with the plugin pushed and the game's submodule commit moved. Steps 1 to 4 need only MyGame. Step 5 needs only Terra Arcana and can come earlier if that suits the work going on there. Sizes are relative.

### Step 0: ground truth and safety nets (small, no gameplay code changes)

1. Create the plugin repo with empty modules and its GitHub remote (the remote on the user's OK). Add it to MyGame as a submodule, build, then **package MyGame and run the smoke test**, since a plugin change can differ in a cooked build. Run `RunUAT BuildPlugin` once to prove the plugin compiles on its own.
2. When Terra Arcana's session is idle: add the submodule there too and build. The plugin can stay enabled in Terra Arcana: its types are prefixed, so nothing clashes with the types Terra Arcana still declares. Clone one game with `--recursive` into a scratch folder and check that the plugin arrives.
3. MyGame: read through the editor, and commit as JSON under `Tests/baselines/`, every `GA_*` class default (the engine's own too: tags, cost, cooldown, triggers), what each of the seven graphs does, the tree, the blackboard, the query, the controllers' defaults and each enemy's abilities. This is the conversion's input.
4. MyGame: extend the ability regression to the enemy and boss abilities and record their baseline.
5. MyGame: record the AI behaviour baseline on the current tree. For each enemy type, many scripted runs (player still, facing away, walking, circling): time to first attack, share of attacks begun behind the player, attackers at once, distance kept, attacks per minute, the wizard's retreat, the boss's move at each health mark. Keep the mean, the spread and reference videos.
6. Terra Arcana: with its tests green, record the results of a fixed set of seeded benchmark fights. Run the set twice to learn whether results repeat exactly.

**Gate:** all existing tests pass unchanged in both games.

### Step 1: combat mechanics out of `AMyCharacter`, still inside MyGame (medium)

Move the mechanics listed in 2.3 into a `UAssCombatComponent` in the MyGame module, under its final name. The ability, the hitbox and the reactions talk to the component and to `IAssCombatant`, not to `AMyCharacter`.

**Gate:** ability regression equal to baseline; `Tests/run_all.py`.

### Step 2: the combat runtime moves into the plugin (medium)

Move the component, the ability class, the hit window task and notify, the hitbox, the hit reaction component, the impact cue and their structs into `AssCombat`, under their `Ass` names (4.1). Add the redirects. Native tags move with them, tag names unchanged.

**Gate:** as Step 1, plus a packaged build and the smoke test.

### Step 3: the shared definition, read by the ability class (medium)

1. Add `FAssAbilityDefinition`, `FAssAbilityInstance` and `UAssAbilityTemplate` to the plugin: Terra Arcana's declarations with their property names unchanged, extended as in 4.2. Add a test that every field survives JSON and back.
2. Change the ability class to read all its data from a definition. MyGame's `GA_*` classes still exist; an editor converter fills each one's definition from its old defaults, and the old properties are removed.

**Gate:** ability regression equal to baseline, player and enemy.

### Step 4: MyGame's abilities run from templates (large)

1. One generic ability class runs any template. It applies the definition's tags, cost and cooldown itself (3.5).
2. The converter writes one template per `GA_*`. Kits become lists of instances with their input button.
3. The seven abilities with graphs become data: projectiles, dashes, self effects and summons. The projectile and dash actions are written against Terra Arcana's `GA_AbilityBase` graph as well, so one implementation later serves both games.
4. A command exports and imports templates and kits as JSON.
5. The old `GA_*` assets are no longer referenced. They are listed for the user and deleted only on their OK.

**Gate:** ability regression equal to baseline; `run_all.py`; packaged build and smoke test; the user plays a run.

### Step 5: Terra Arcana's data types and pattern AI move into the plugin (large, no behaviour change)

1. Replace Terra Arcana's own ability data structs with the plugin's, first adding any field Terra Arcana gained since Step 3. Because the names differ (4.1), this can go one type at a time, with a build and the tests in between.
2. Move the AI structs, the runner, the rules, the checks and the rule tests into `AssPattern`. Split `UTerraPatternComponent`: the general part goes to `AssDriver`, the fight log and arena parts stay in a Terra Arcana child.
3. `UTerraAbility` and `GA_AbilityBase` keep running Terra Arcana's abilities, reading the same fields from the plugin's struct.
4. Rename no serialized property or enum value. Keep the primary asset type `AbilityTemplate`, and point its scan entry in `DefaultGame.ini` at the plugin's class. Add redirects. Bump `ReplayVersion`.

**Gate:** the four test commands; the benchmark set equals Step 0; a stored Lair's JSON and the tuning JSON come out unchanged.

### Step 6: the pattern system gains what MyGame's enemies need (medium)

Add the items of 4.3 to the runner, the snapshot, the checks and the driver, each with rule tests in the plugin.

**Gate:** the plugin's rule tests; Terra Arcana's tests and benchmark set unchanged; `AIRevision` bumped only if an existing rule changed.

### Step 7: MyGame's enemies run on patterns (large)

1. Write patterns for the base enemy, the kicker, the wizard and the boss, from the facts recorded in Step 0.
2. Put the driver on `AMyAIController`, which keeps its sight and crowd avoidance. The tree stops running.
3. Compare against the Step 0 behaviour baseline and send the user before and after videos. Repeat until both agree.
4. The tree, query and task assets stay until the user OKs their deletion.

**Gate:** the behaviour measures inside the recorded spread; ability, loop and level regressions; packaged build and smoke test; the user's eye.

### Step 8: the combat module becomes replication-correct (medium)

Gate game logic on the authority. Make everything a viewer sees replicated state or a cue: the tags now added loosely, hit pause, body-block state, knockback and launch. Remove any dependence on wall-clock time or frame rate. Bump `CombatRevision` only if MyGame's results change, which they shouldn't.

**Gate:** MyGame's ability regression equal to baseline at normal speed, at a faster speed and on a fixed 1/30 s step; a review of the module against the replay rule. The full proof is Step 9.

### Step 9: Terra Arcana's abilities run on the shared runtime (large)

1. The plugin's ability class gains what `UTerraAbility` and `GA_AbilityBase` do that MyGame's class didn't: fitting the montage to times, timed and toggled self effects, projectile spreads, the slide after a dash, levels, the reasons an ability ended, loading assets when granted.
2. `UTerraAbility` becomes a thin child holding Terra Arcana's own rules, or goes away. The `GA_AbilityBase` graph is retired, with the user's OK for the asset.
3. The combat component goes on Terra Arcana's character, with poise feeding it.

**Gate:** the four test commands; the benchmark set equals Step 0, because existing templates haven't changed; replays record and play; `ReplayVersion` bumped.

### Step 10: the first advanced abilities in Terra Arcana (medium, new content)

Make three templates as the proof of goal 2: an ability combo with a cancel, a launcher, and a dash that turns body blocking off. Bake reach and timing so the bot, the pattern checks and the tuning table can read hit-window abilities, and teach `ToTuningJson` to count hit payloads. Bump `CombatRevision` and `ReplayVersion`, and push the tuning.

**Gate:** Terra Arcana's tests plus new ability tests; a recorded replay shows the pause, the knockback and the launch as the live fight did; the user judges the feel from clips.

---

## 6. Risks, and what to verify early

| Risk | Why it matters | Handling |
|---|---|---|
| Two working copies of the plugin | A change left unpushed in one game's copy, or made in both, diverges | One session edits the plugin at a time; every step ends pushed (section 5). |
| Submodule left behind | A clone without `--recursive`, or a pull that doesn't update the submodule, builds against a missing or old plugin | Each game's README and build script run `git submodule update --init`. |
| Per-definition tags in GAS | The engine reads tags from the ability's class | Own step (4.1), gated by the regression. Fallback: a template names a subclass. |
| Types change name when they move (the `Ass` prefix) | Blueprints, assets and config refer to the old names. Stored ability ids carry the primary asset type. | Redirects in each game. The primary asset type stays `AbilityTemplate`. JSON holds property and enum value names, which don't change (4.1). The prefix also means no plugin type clashes with a type a game still declares. |
| Terra Arcana's structs change between Step 3 and Step 5 | Another session is working there | Step 5 starts with a diff. Doing Step 5 first removes the risk. |
| Stored JSON | Property and enum value names are in server rows | No serialized name is renamed. Generalizing names such as `Titan` and `Raider` is left out. |
| The tree is unseeded | No move-for-move comparison is possible | Statistical baseline and the user's eye (1.1). |
| `GA_AbilityBase`'s logic is a Blueprint graph | It must be read in the editor to be ported | Benchmark equality gates the port. |
| Hit pause and replays | `CustomTimeDilation` isn't replicated, and hidden benchmark worlds run far faster than real time | Designed in Step 8, proven in Step 9. |
| Facts in section 2 not confirmed in the editor | The tree's shape, which abilities have graphs | Step 0.3 reads them before anything is converted. |

## 7. Decisions for the user

1. **Names: decided 2026-11-12.** Modules and types carry the `Ass` prefix (3.3, 4.1). The exact type names in 4.1's table are a proposal under that rule, cheap to change before Step 0 and costly after Step 2.
2. **Location: decided 2026-11-13.** A git submodule in each game's `Plugins` folder (3.2). It needs a GitHub repo for the plugin, created only on the user's OK: private or public?
3. **`MIGRATION.md` Phase 7's "AI on StateTree"** is replaced by Step 7.
4. **Boss moves.** A pattern phase retries a move that couldn't start; today's event response doesn't. The plan keeps the retry and lists it as the one known difference, unless the user wants the old behaviour exactly.
5. **Leftover assets:** `GA_Test`, `GA_Test2`, `GA_VoadoraOld`, and the two assets both named `GA_DamageEnemy1`. Convert them or leave them out?
6. **Order.** MyGame first (Steps 1 to 4, then 5), as written, or Terra Arcana's types first (Step 5 before 3).
