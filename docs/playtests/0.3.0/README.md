# Playtests for 0.3.0

## AI playtest, run 3 (2026-10-28)

Three AI agents played commit `88acead` (0.2.0, with its save fixtures) through
[playtester](https://github.com/Preponderous-Software/playtester)'s `sisyphus-strata` profile: headless on the Strata
driver, with a stepped clock, steering by screenshots, in first person, the default since 0.2.0. Run 2's goals were
kept for the new player; the other two were rewritten to exercise what 0.2.0 added:

| Session | Goal | Reached | Fun |
| --- | --- | --- | --- |
| s01 | a brand-new player: work it out from the screen and climb | 1.0, the summit | 4/5 |
| s02 | upkeep with dependencies, refactoring, retiring and stamina | 0.3 (the goal) | not given |
| s03 | the whole climb, resting when tired, then read the cairn for 1.0 | 0.6, 5 m short of the summit | 3/5 |

The new player reached the summit, which no session had before ([run 2](../0.2.0/README.md): 0.1, 0.2 and 0.5). The new
mechanics read clearly: an outage showed which modules used the one that was down, patching the cause cleared it, and
s02 preferred refactoring to retiring because it lost no users. What still held them back:

- finding the module named in the issue list on the stone, especially underneath (all three);
- walking past the boulder instead of pushing it when not lined up (s01);
- with many issues open, the list printed over the prompt (s03);
- users leaving and modules rotting while the boulder rolled down after the summit, when nothing can be done (s01).

The machine was shared and ran several times slower than game time, so minutes in the reports are wall-clock. The full
reports, findings and screenshots are in [ai-playtest/](ai-playtest/), starting with [summary.md](ai-playtest/summary.md).
The run was made on 0.2.0; contributors, the history screen, the rewrite at the summit and weather came after it and
were not in the build played.

### What was changed for it

| Finding | Change |
| --- | --- |
| Can't find the module named in the issue list (s01, s02, s03) | Each open issue in the HUD says where the module is as he sees it: "facing you", "far side", "on your left", "on your right", "on top", or "underneath (roll it)". |
| Walks past the boulder instead of pushing it (s01) | Walking straight into the boulder without being lined up to push now stops him against it, instead of sliding off round its side and on up the path, and the prompt says to put it in the middle of the view. Walking past it at a slant still goes round. A test walks into it from the side. Loosening the push itself was tried and dropped: the autopilot then pushed it by accident while working round it. |
| The issue list prints over the prompt (s03) | The HUD stops above the prompt, ending in "... and N more lines". |
| Decay while it rolls down after the summit (s01) | Rot and users leaving pause while it rolls back to the trailhead. |
| Rolling it back on a terrace raises the "rolling back down!" alarm (s01) | The alarm shows only on a slope. At the terrace's edge the prompt says it rolls back no further. |
| "Hold E" on modules that look healthy; "rotting" modules not in the issue list (s01, s02) | A module with no issue reads "nearly new" (90-98%) or "aging, no issue yet" (below 90%). |
| Stamina never felt like a decision; sitting hides the HUD (s02, s03) | Sitting shows stamina coming back under the quote, and an outage still shows while he sits. |
| "1 m uphill" when nearly there; the next-release line cut off at 640 px (s01, s03) | It says "under 1 m", and the line starts "Next 0.4:" instead of "Next release 0.4:". |

Not changed: module labels only appear on the side facing him and fade up close; the position column now carries that
job. A module at the contact point has to be rolled round, as the HUD says.
