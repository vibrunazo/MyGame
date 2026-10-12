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
- Test in PIE with the regressions (`Tests/*_regression.py`). Package plus `Tests/package_smoke.py` only for changes that can differ in a cooked build (asset references, config, plugins, startup and menu flow, input mode), or when the user wants an exe.
- The user judges feel (camera, audio) by eye and ear: send videos or clips, not just numbers.
