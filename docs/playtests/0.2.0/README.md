# Playtests for 0.2.0

## AI playtest, run 2 (2026-10-28)

Three AI agents played commit `8ce3be2` (main after 0.1.0, with the fixes from [run 1](../0.1.0/README.md)) through
[playtester](https://github.com/Preponderous-Software/playtester)'s `sisyphus-strata` profile: headless on the Strata
driver, with a stepped clock, steering by screenshots. The goals were the same as run 1's:

| Session | Goal | Reached | Fun |
| --- | --- | --- | --- |
| s01 | a brand-new player: work it out from the screen and climb | 0.1 | 2/5 |
| s02 | upkeep: stay on a terrace and keep it alive, turn it, retire a module | 0.2 (the goal) | 3/5 |
| s03 | the whole climb to 1.0 and back, then sit | 0.5 | 3/5 |

They got further than run 1 (0.1, 0.1 and 0.6, with only s03 past 0.1), and all three again read the
metaphor unprompted and found the rot fair and readable. What still held them back:

- pushing from a terrace slid them off the boulder's side, and it rolled back (all three);
- they could not tell which way was uphill once they had walked round the boulder;
- stopping on a slope to patch let the boulder roll back, with no warning;
- after 0.4, trouble came faster than one player could patch it (s03);
- a held R went straight on to retire the next module (s02).

They played the over-the-shoulder camera, which was the default then; the game has since gone back to first person,
with V for a view from straight behind. The machine was shared with other work and ran several times slower than game
time, so minutes in the reports are wall-clock. The full reports, findings and screenshots are in
[ai-playtest/](ai-playtest/), starting with [summary.md](ai-playtest/summary.md).

### What was changed for it

| Finding | Change |
| --- | --- |
| Pushing slides him off round the boulder (s01, s02, s03) | The cause: while pushing, his forward input still moved him sideways by the sine of how far he was turned (0.75 m/s at 20°), faster than squaring up drew him back (0.6 m/s). Pushing now leans him straight into the boulder, with no sideways drift and no strafing. The push-from-a-terrace test now also pushes turned up to 0.6 rad either way; without the change three of those fail. |
| No way to tell which way is uphill (s01, s03) | The next-release line says where uphill is when it is not ahead: "(to your left)", "(to your right)", "(behind you)". |
| Patching on a slope lets it roll back, unwarned (s01, s02, s03) | On a slope, the patch prompt opens with "On the slope it rolls back if you stop: patch on a terrace". The rolling-back message says to let go only on a terrace. |
| Overwhelmed after 0.4 (s03) | New incidents come more slowly while issues are open (the rate is divided by 1 + 0.25 per open issue). Over seeds 1-8 the autopilot reaches 1.0 in 4.8-5.7 minutes and 2.0 in 16-28, with fewer modules going down. |
| A held R retires a second module (s02) | A retire or refactor that finishes makes R and F wait to be let go before they work again. |
| The "0.2" label draws over the prompt (s02) | Terrace labels are not drawn over the prompt. |
| The weight hint runs off the screen (s03) | Shortened to "(rot weighs it down: patch, retire or refactor)". |
| Modules on top cannot be aimed at from the outside camera (s02); the target followed his eye, not the crosshair (s01) | First person is the default again, and V now looks from straight behind him, so the crosshair is where he looks. |

Not changed: Shift does not speed up pushing (s03). Hurrying is for walking; pushing goes at the boulder's pace.
