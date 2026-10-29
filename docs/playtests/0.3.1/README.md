# Playtests for 0.3.1

## AI playtest, run 4 (2026-10-28)

Three AI agents played commit `6954b5d` (0.3.0) through
[playtester](https://github.com/Preponderous-Software/playtester)'s `sisyphus-strata` profile: headless on the Strata
driver, with a stepped clock, steering by screenshots, in first person. Run 3's new-player goal was kept; the other two
were written for what 0.3.0 added:

| Session | Goal | Reached | Fun |
| --- | --- | --- | --- |
| s01 | a brand-new player: work it out from the screen and climb | 0.6 | 3/5 |
| s02 | the long haul: helpers, weather and the history screen | 0.6 | 3/5 |
| s03 | legacy or rewrite: reach the summit, choose a rewrite, climb on | 0.5 | 3/5 |

None reached the summit. The machine was shared and ran the game six to seven times slower than game time (s03: 20 s
of pushing took 142 s), so a release took several real minutes and the summit goals did not fit their budgets. The
rewrite and the 1.0 cairn were not reached, and no session saw rain. What they did see:

- **A regression from run 3's fixes.** Keeping the HUD above the prompt cut the list of open issues to "... and N more
  lines" once a helper, rain, stamina or an outage line was showing, so all three lost track of which modules were sick.
- **Outages:** the HUD said "OUTAGE: users are leaving" without naming the module that was down (s01, s03).
- **Tired on the last slope:** with the boulder heavy and stamina low, the push crawled, and sitting to rest (which the
  game suggested) let it roll back a terrace (s01, s03).
- **Helpers:** a helper's time ran out while they were still walking up from the town (s02). Leaving went well: "Noor
  moved on. Nobody else knew how core worked." and core drifted.
- **History:** clear, though the "min" column was not explained (s02).

The full reports, findings and screenshots are in [ai-playtest/](ai-playtest/), starting with
[summary.md](ai-playtest/summary.md).

### What was changed for it

| Finding | Change |
| --- | --- |
| The open issues vanish behind "... and N more lines" (s01, s02, s03) | Short of room above the prompt, the HUD drops its optional lines first (the weather, the weight hint, a helper, stamina when not tired, the spacer), so the open issues always show; only then does it cut the list short. Checked at 640x360 with a helper and two issues. |
| Issue rows run off the right edge at 640 px (s01, s02, s03) | Shorter issue names ("drift", "deprecated", "certificate") and positions ("left", "right", "under: roll it"), so a row fits. |
| The outage never names the module (s01, s03) | The HUD says "OUTAGE: router is down (far side)", and so does the message when it goes down. |
| Sitting on the slope rolls it back (s01, s03) | The stamina hint says to rest on a terrace, and pushing tired on a slope says to push on to the terrace, because sitting there lets it roll back. While sitting, the boulder rolling back shows on screen. |
| Tired and heavy, the push crawls (s01) | Spent, he keeps 70% of his strength (was 55%). |
| A helper's time runs out on the walk up (s02) | A helper's time counts only while they are beside the boulder. |
| The history's "min" column (s02) | A line under the table says what it is. |

Over seeds 1-8 the autopilot reaches 1.0 in 4.7-8.2 minutes and 2.0 in 12.6-30.2. The pinned playthrough ends on
tick 8516.

Not changed: a module where the boulder meets the ground still has to be rolled round to be reached (s02). The issue
list now says "under: roll it" for it.
