# MyGame's combat: its clocks, its order in a frame, and what will replicate

Ass Plan.md Step 1, item 1: written before the combat mechanics left `AMyCharacter` for `UAssCombatComponent`, so the
move kept every timer on the clock it used and the frame's order as it was. Since Step 2 the combat runtime is the
AbilityStrategySystem plugin's (its `AssCombat` module), under the plugin's names, which this file uses; MyGame keeps its
rules (the play rate, `UMyAbilityPolicy`) and its presentation (the slash, the strike sounds, the shake).
The names below are the component's since item 4; `AMyCharacter` calls its hooks at the points it
called its own code before (the start of `BeginPlay`, after its ability system knows its actor, the end of
`BeginPlay`, its movement mode changing, its death). Read from the code on 2026-11-17
(`ue58`, after the move to the UE5 mannequin); the strict baseline (`Tests/baselines/strict.json`) holds the result.

## The clocks

1. **World time**, `UWorld::GetTimeSeconds()`: the game's clock, scaled only by the world's time dilation. It has
   already moved to the frame's end when actors tick (a frame covers world time `T1 - DeltaSeconds .. T1`). Engine
   timers (`FTimerManager`) and gameplay effects' durations and periods run on it.
2. **An actor's time**, world time times the actor's `CustomTimeDilation`: the delta an actor's own tick and its
   components get (the character's tick, `UCharacterMovementComponent`, the skeletal mesh and its anim instance). The
   hit pause is this clock slowing down: `UAssCombatComponent::OnFrameStart` sets the owner's `CustomTimeDilation` every frame (1, or
   `HitPauseRate` = 0.01 while paused, or a fraction in the frame a pause starts or ends, with a debt so the frame a
   hit lands in pays back what it ran past the hit).
3. **Montage position**, `UAnimInstance::Montage_GetPosition`: the montage's own time. The anim instance advances it
   by the actor's delta times the montage's play rate times its `RateScale`. The play rate is set when the ability
   starts the montage: the game's policy, `UMyAbilityPolicy` (the `AttackSpeed` attribute times the ability's
   `MontagesSpeed`), through `UAssAbility::GetPlayRate()`.
4. A pattern's time: MyGame has none yet (the behaviour tree runs on the AI controller's tick, world time); it comes
   with Step 7.

## Which clock each one uses

| What | Where | Clock |
|---|---|---|
| A hit window's start and end | `UAssHitboxNotifyState` notify begin and end (the `Ass.Notify.Hit.Start` / `Ass.Notify.Hit.End` events to `UAssHitboxWindowsTask`); the window's own times come from the notify's place in the montage (`UAssHitboxNotifyState::GetWindow`) | montage position |
| A window's samples | `AAssHitBox::SampleWindowUntil`: every 1/60 s of montage position (`WindowStep`) from the window's start, the last one cut to its end; each shape posed from the montage's animation data at that position (`MontageBoneInComponent`), never from the mesh's bones | montage position |
| A sample's moment | the same sample, placed between the frame's start (`FHitboxPose`, captured in `OnFrameStart`) and now by its montage position: the attacker's and the target's transforms are interpolated to it, and it carries a world time | world time, interpolated |
| `NumHits` and `HitCooldown` per target | `AAssHitBox::TryHit`, with the sample's world time | world time |
| Hit pause | `UAssAbility::OnHitConnect` asks for `HitPause` seconds; `OnHitPause` starts it at the hit's sample time (`NoteHitConnect`), `OnFrameStart` turns it into `CustomTimeDilation` frame by frame | world time, felt as actor time |
| Niagara on the mesh during a pause | `SetHitPauseFX` pauses them while paused | follows the pause |
| Combo reset | `UpdateCombo`: world time past `LastComboTime + ComboResetDelay`; `LastComboTime` is the frame's world time when a hit connects (not the sample's) | world time |
| The cancel window (`combo.cancancel`) | set on hit connect and at a window's end, cleared when an ability starts and when its montage ends | events, on montage position |
| Hit stun | `GE_HitStun`'s duration (`Ass.Data.HitStun`); its reaction plays the GetHit montage, under the victim's dilation | world time |
| Stun count and immunity | `IncrementHitStunCount` on every landed stun; at `MaxStuns` it applies the component's `StunImmuneEffect` for `StunImmuneCooldown` seconds (`Ass.Data.StunImmune`). The count never decays | world time (the effect) |
| Knockback | `ApplyKnockBack` sets ten timers at `i * 0.01` s for i = 0..9; the engine sets none at rate 0, so nine fire, each moving the victim 1% of the vector along the attacker's facing (`AddActorWorldOffset`, swept, no gravity) | world time (timers) |
| Launch | `ApplyLaunchBack`: `LaunchCharacter` after a 0.05 s timer, and the component's `NoControlEffect` for 0.5 s (`Ass.Data.NoControl`) | world time |
| Body block | `RefreshPawnCollision` on the tag `Ass.Status.NoPawnBlock`, movement mode and death; inside another character it retries every 0.05 s and steps out after 0.3 s | world time (timers) |
| Airborne tag and the ground/air ability blocks | `OnMovementModeChanged` | movement events |
| Root-motion speed curves | `ApplyRootSpeedCurve` in `OnFrameStart`: the `RootSpeedX` curve over the montage time this frame plays, as a root motion source for the frame's movement time (`DeltaSeconds * CustomTimeDilation`) | montage position and actor time |
| The Tatsumaki's spin | `UAssRotateNotifyState::NotifyTick`, from the montage position | montage position |
| Death | the body launches after a 0.05 s timer; `OnDelayedDeath` after 5 s | world time |
| Ability cost and cooldown | the engine's `CommitAbility`, gameplay effects | world time |

## A frame, in today's order (kept by the move)

1. **`FWorldDelegates::OnWorldPreActorTick`**: every character's `OnFrameStart`, in the order they began play.
   It records where the frame starts (`FHitboxPose`: world time `T1 - DeltaSeconds`, montage and position, mesh and
   actor transforms), sets this frame's `CustomTimeDilation` from the hit pause, and applies the root speed curve.
2. **`TG_PrePhysics`**:
   - the character's tick: held ability slots retry (`ActivateAbilityByInput`), so an ability starts here;
   - movement with the actor's delta and its root motion sources;
   - the skeletal mesh and anim instance: the montage advances and its notifies fire. A window's start event
     spawns the `AAssHitBox`, which samples at once up to the montage's position. `UAssApplyEffectNotifyState` applies its
     effects here.
   - after the pose: `OnBoneTransformsFinalized` calls `UAssCombatComponent::OnPoseFinalized`, and every open window samples
     up to the montage's new position. A hit applies its effects one by one, in the ability's order: damage and what
     it sets off (death, the health bar, the health marks' events, the boss's response), then stun, knockback and
     launch through `UAssHitReactionEffectComponent`. The attacker's hit pause starts at the hit's sample time.
3. **`TG_StartPhysics` to `TG_PostPhysics`**: nothing of combat's.
4. **Latent actions, then the world's timers**: the knockback's offsets, the launch, the body-block retry, effect
   expirations.
5. **`TG_PostUpdateWork`, `TG_LastDemotable`, `OnWorldPostActorTick`**.

Within a tick group the engine leaves the order between characters to chance; in tests the strict recorder fixes it
(`CombatRecorder.h`). The game itself has relied on nothing there that the baselines don't hold.

## What the combat component will replicate (Step 8)

From Step 0b's replay slice (Ass Plan.md 3.9), and what this code adds:

- **The hit pause**: its start (world time), duration and rate, applied on every machine as the actor's
  `CustomTimeDilation`, which doesn't replicate. The debt rule (a frame that ran past the hit pays it back) is
  computed per machine from those three.
- **Windows advance from a tick or the montage's advance, never from the pose event.** Today they advance from
  `OnBoneTransformsFinalized` (`OnPoseFinalized`); the samples already read bones from the animation data, not the
  mesh, so moving the call is what's left. That change waits for Step 8: moving it now could move a hit by a frame.
- **Hits on the authority only**: `TryHit`, the effects and the reactions. Effects replicate through the ability
  system; the impact cue reads its sounds and particles from the hitbox actor, which the window's end destroys.
- **Knockback and launch** move the character on the authority with timers (offsets, `LaunchCharacter`): a
  predicting client would fight them. Step 8 makes them replication-correct and keeps their feel.
- **Derived on each machine, not replicated**: the airborne tag (a loose tag, from the movement mode), the body
  block (from tags, movement and death), the frame's start pose, the root speed curve's source.
- **Already replicated by the ability system**: stun, stun immunity, no control (effects and their tags), damage and
  death through the attributes. The stun count is the authority's alone.

## `IAssAbilityPolicy`, its first form

Step 2 needs one thing from a game's rules: **the rate an ability plays at.**

```cpp
// The play rate of Ability's montage for its avatar, read when the ability starts its montage
virtual float GetAbilityPlayRate(const UGameplayAbility& Ability, const AActor& Avatar) const = 0;
```

MyGame's answer, `UMyAbilityPolicy` (named in `DefaultGame.ini`), is Step 1's `GetAttackSpeed()`: the avatar's
`AttackSpeed` attribute times the ability's `MontagesSpeed`, 1 without attributes. It's read once per activation; a change while the montage plays
waits for the next one. The root speed curve reads the montage's actual rate, so it follows.
