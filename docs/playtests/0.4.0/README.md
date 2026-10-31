# Playtests for 0.4.0

## AI playtest, run 6 (2026-10-30)

Run 5 again, on the **stepped clock**. Four AI agents played commit `9a68cbf` (`main`: 0.3.1 with #30 `--from`, #31
and the #32 docs) through [playtester](https://github.com/Preponderous-Software/playtester)'s `sisyphus-strata`
profile (playtester `0561475`), headless on the Strata driver at 640x360, steering by screenshots. Every session was
started with `--clock stepped`, so the world moved only while an agent held a key or waited, never while it read a
screenshot. Everything else was kept from run 5: the four goals, the seeds and the three start saves (loaded with
`--from`), so the clock is the one thing that changed. The balance was not touched between the runs. The harness smoke
(`pt smoke --clock stepped`) printed PASS, all four reports validated, and `pt aggregate` reported no problems.

| Session | Start | Goal | Outcome | Fun (run 5) |
| --- | --- | --- | --- | --- |
| s01 | 0.6, on the terrace below the summit | ship 1.0 and choose the rewrite | achieved: 1.0 shipped, rewrite chosen, 1.1 shipped after it | 3/5 (2) |
| s02 | 1.1, a shower late in the afternoon | climb through the rain into nightfall | achieved: 1.2 and 1.3 shipped at night, no module down, users 570 to 740 | 3/5 (2) |
| s03 | 0.2, a helper just come up from the town | the helper, then the History screen | achieved: 0.3 and 0.4 shipped, the helper watched to the end | 4/5 (2) |
| s04 | a new game | a brand-new player | partial: 0.1 to 0.5 shipped, then lost the boulder on a terrace | 3/5 (2) |

The sessions played 226 to 248 seconds of game time each (s04: 13,660 frames) over 20 to 28 minutes of wall time.

**The run 5 spiral did not recur.** No module went down in any session, and no boulder became
too heavy to push. A brand-new player shipped five releases where run 5's never reached 0.1, and the s02 save that
spiralled from 570 users to 1 in run 5 shipped two releases and gained users. Run 5's balance findings came from the
free clock, so this run gives no reason to change the pace of rot or the push.

**Findings:** 0 blocker, 2 major, 14 minor and 12 polish (28 reports in 24 groups, three of them praise). Both majors
were checked against the screenshots and the source, and an issue was filed for each:

- **After the rewrite, the roll-down message describes the other choice** (s01): it says "The boulder rolls. Every
  module goes with it." whatever was chosen, and the HUD still shows 13 modules and 500 users on the way down, so
  nothing confirms the rewrite until the trailhead ([after5.png](ai-playtest-run6/s01/shots/after5.png)). Filed as #33.
- **"The boulder is behind you, down the hill" shows while the boulder is in view** (s04, and s01 at the trailhead): the
  lost-boulder prompt added in #31 fires whenever the boulder is lower than he is, whichever way he faces. On a terrace,
  walking round it to roll it back sets it off, and s04 spent about 40 game seconds circling it while a module rotted
  ([back12.png](ai-playtest-run6/s04/shots/back12.png), [nav_fail.png](ai-playtest-run6/s04/shots/nav_fail.png)). Filed
  as #34.

The minors, not filed (details in [summary.md](ai-playtest-run6/summary.md)):

- **Finding the module an issue names.** "right" and "far side" turn as he circles the boulder, and three or more issues
  collapse into "... and N more lines" (s02).
- **The helper's timer.** "on the way up, 2:30 left" does not count down while the helper walks up, and it reads as if
  they are still in the town (s02, s03, s04). The timer stopped during the roll down (s01) but ran on one slope and not
  on another (s03).
- **History.** Mid-climb it is one row of numbers ("0.x" for the current climb). A rewritten climb shows only the users
  and modules from before the rewrite (s01, s03, s04).
- **Weather and night.** The shower's "Rain: the path is slick" arrives about 16 s after the load and leaves without a
  word. At night the boulder and the hill are near-black, though modules with open issues glow clearly (s02).
- **Smaller ones.** The summit choice has no countdown, and R is also retire (s01). A push slightly off-centre does
  nothing, with no hint, because the module prompt takes the place of "Not lined up" (s02).
- **Text.** HUD lines are clipped at the right edge, and "-60%/min left" reads as time left rather than the side of the
  boulder (s01, s03, s04, polish).

What landed: the summit choice text and the hold-R bar (s01); "Rewritten. 6 fresh modules, and most of the users did not
follow." (s01); the helper's status lines and "Kofi moved on. Nobody else knew how search worked.", followed at once by
a drift issue on search (s03, its best moment); and the onboarding, pushing, releasing and patching, which a new player
got right first time (s04). All four agents read the game as the endless upkeep of software, with terraces as releases
and rot as drift and decay (answers in [summary.md](ai-playtest-run6/summary.md)).

Nothing was changed for this run: it is evidence only. The full reports, findings and cited screenshots are in
[ai-playtest-run6/](ai-playtest-run6/). Local paths are replaced with `<run>`, `<sisyphus checkout>` and `<saves>`.

## AI playtest, run 5 (2026-10-29)

Four AI agents played commit `8ef2a7f` (0.3.1 with `--from`) through
[playtester](https://github.com/Preponderous-Software/playtester)'s `sisyphus-strata` profile, headless on the Strata
driver, steering by screenshots. Run 4 never reached the summit, the rewrite or the rain, so three of the agents
started from saves of the autopilot's own games, loaded with `sisyphus --from DIR`:

| Session | Start | Goal | Outcome | Fun |
| --- | --- | --- | --- | --- |
| s01 | 0.6, on the terrace below the summit | ship 1.0 and choose the rewrite | blocked, never left 0.6 | 2/5 |
| s02 | 1.1, a shower late in the afternoon | climb through the rain into nightfall | partial, about 9 m climbed | 2/5 |
| s03 | 0.2, a helper just come up from the town | the helper, then the History screen | partial, no terrace reached | 2/5 |
| s04 | a new game | a brand-new player | partial, 12 m of the 33 m to 0.1 | 2/5 |

**The run was not a fair test of the balance.** The sessions ran on the free clock, not the stepped clock that runs 3
and 4 used, and the machine was busy, so the game ran at 6 to 20 frames a second while each agent took tens of seconds
to read a screenshot and decide. The game kept time with the real clock, so the modules rotted for those tens of
seconds at every step. All four ended in the same spiral: modules down, users gone, the boulder too heavy to push.
In run 3, on the stepped clock, a new player reached the summit, so this spiral is mostly the clock. What the run did show,
whatever the clock:

- **Work lost on letting go.** Holding E, R or F for a few seconds left no trace once the key was released, so short
  holds looked broken and refactoring (10 s) seemed to do nothing (s01, s02, s03, s04).
- **Losing the boulder.** When it slipped away, he walked on uphill and nothing on screen said so (s01, s02, s03, s04).
- **Weight unexplained.** Nothing said what made it heavy or how to lighten it; retiring healthy modules made it
  worse (s01, s02, s03).
- **No moment when the last user leaves (s01, s03), and the helper's line vanished when issues piled up (s03).**
- **"aging, no issue yet" in the prompt for a module the HUD lists as "rotting" (s04).**

What landed: the writing ("It will not last.", "The boulder is still. For a moment, so are you.", "Kofi moved on.
Nobody else knew how search worked."), the helper walking up and patching, and the History screen (s01). The full
reports, findings and screenshots are in [ai-playtest/](ai-playtest/), starting with
[summary.md](ai-playtest/summary.md).

### What was changed for it

| Finding | Change |
| --- | --- |
| Short holds do nothing; refactoring seems broken (all four) | Letting go of E, R or F pauses the work instead of starting it over: holding again on the same module goes on from there. The prompt names the key with its bar ("Patching (hold E)"). Work on another module, or of another kind, starts over. |
| The boulder slips away unseen (all four) | Walked on uphill without it, the prompt says "The boulder is behind you, down the hill: go back down to it". |
| Weight is a mystery (s01, s02, s03) | Once it is heavy, the HUD says what it is made of: "Down modules weigh most: patch or retire", or "Rot weighs: patch the oldest". |
| No moment at 0 users (s01, s03) | "Every user has left. It still runs, for nobody. Ship a release and some will come." |
| The helper's line vanishes (s03) | It is no longer dropped when the HUD is short of room. |
| "aging" in the prompt, "rotting" in the HUD (s04) | The prompt says "rotting, no issue yet". |

Letting go no longer resets the work, so the autopilot's playthrough moved: it ends on tick 8516 with fingerprint
`9e53ddbdb04e2e8f`. Over seeds 1-8 it reaches 1.0 in 4.7-8.2 minutes and 2.0 in 13.1-28.5 (before: 12.6-30.2).

Not changed: the pace of rot and the push itself, until a run on the stepped clock shows whether they are a problem
when the game waits for the player. The run's starting saves were also unkind: s02's started with core on top of the
boulder, out of reach.
