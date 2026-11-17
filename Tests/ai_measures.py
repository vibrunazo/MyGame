"""Step 0c item 4: the AI measures, and the tolerances Step 7 is held to (Ass Plan.md, Step 0c and 1.1).

Fixed here, before any number is computed and before anyone sees the port (committed on its own first).

What chance decides (which spot the query picks, when a wait ends against the player's movement) is measured over many
seeded runs; the decision matrix and the boss scenarios (ai_behaviour.py) carry what is exact.

RUNS
  Solo: each enemy type (TYPES) alone against the player, in each player behaviour (BEHAVIOURS), for each seed in
  SEEDS, SOLO_SECONDS long. Group: the mixed group GROUP (three enemies at once), in each behaviour, for each seed,
  GROUP_SECONDS long. Each run spawns its enemies fresh at the same spots, wakes them with the player as their target,
  and is seeded (run_timeline's seed: every tick reseeded from the seed and the tick's number). The player has health
  and defense enough never to die, and never attacks.

BEHAVIOURS (the player's)
  still     stands facing the enemies' side
  away      stands with its back to them
  walking   walks back and forth across the room, 2 s each way
  circling  walks a square around the room's middle, 1.5 s a side

MEASURES (per run; then per cell, a cell being type x behaviour, or the group x behaviour)
  first_attack   seconds from waking to the enemy's first ability start (the run's length if none)
  behind_share   of its ability starts, the share begun with it behind the player (more than 90 degrees from the
                 player's facing)
  longest_gap    the longest time without any enemy starting an ability, in seconds
  distance       the mean distance between the enemy and the player, in cm
  per_minute     its ability starts per minute
  retreat        the wizard only: the mean of its distance change over the 0.5 s after the player comes within its
                 aggressive range (positive: it backs away)
  at_once        group only: the mean over frames of how many enemies are in an ability at once
  token_changes  group only: how many times the player's attack token (its aggro list) changes hands
  stunned_hold   group only: the mean time a token holder keeps the token while stunned, in seconds

THE TOLERANCE RULE (Step 7 against this reference)
  For each cell and measure: the reference gives the mean m and the standard deviation s over its N runs (N = the
  seeds, one run each); Step 7 runs the same cells with the same seeds and gives m2 and s2. It passes when both hold:
    |m2 - m| <= max(3 * sqrt((s^2 + s2^2) / N), FLOOR[measure])
    s2 <= 2 * s + FLOOR[measure]
  Three standard errors keeps chance failures rare over all the cells (about 0.3% a comparison); the floor keeps a
  cell whose runs all agree (s = 0) from demanding the same number to the decimal. The reference is recorded twice, in
  two sessions; it is only taken if the two agree on this same rule, so the band covers whatever varies between
  sessions.
  Reference videos: one run per type and behaviour (seed SEEDS[0]), kept beside the reference for the user to compare
  by eye with the port's.

Who takes the token when two enemies qualify on one tick is exact, not measured: a scenario puts two enemies in range
on the same tick, run in both orders of the recorder's roles (it orders their ticks), and records who holds the token.
"""

TYPES = ("enemy1", "enemy2", "enemy3", "kicker", "wizard")
GROUP = ("enemy1", "enemy2", "kicker")
BEHAVIOURS = ("still", "away", "walking", "circling")
SEEDS = tuple(range(1, 11))
SOLO_SECONDS = 10.0
GROUP_SECONDS = 15.0
FLOOR = {"first_attack": 0.25, "behind_share": 0.1, "longest_gap": 0.5, "distance": 25.0, "per_minute": 3.0,
         "retreat": 25.0, "at_once": 0.25, "token_changes": 1.0, "stunned_hold": 0.25}
