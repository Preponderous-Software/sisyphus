# Playtests for 0.1.0

## AI playtest, run 1 (2026-10-28)

Three AI agents played commit `a4a953e` through [playtester](https://github.com/Preponderous-Software/playtester)'s
`sisyphus-strata` profile: headless on the Strata driver, with a stepped clock, steering by screenshots. Each had a
different goal:

| Session | Goal | Reached | Fun |
| --- | --- | --- | --- |
| s01 | a brand-new player: work it out from the screen and climb | 0.1 | 2/5 |
| s02 | upkeep: stay on a terrace and keep it alive, turn it, retire a module | 0.1 | 3/5 |
| s03 | the whole climb to 1.0 and back, then sit | 0.6 (14 m short of the summit) | 3/5 |

All three read the metaphor without being told it. The boulder is the codebase, terraces are releases that add load,
and rot is decay that happens even to code nobody touches. All three found the rot itself fair and readable from the
HUD. What held them back was physical:

- they lost the boulder, and could not get back behind it once it rolled;
- they could not find the module named in the HUD on the stone;
- the progress bar lingered after the work ended;
- nothing explained what makes the boulder heavy.

The machine ran several times slower than game time, so the minutes in the reports are wall-clock time. The full
reports, findings and screenshots are in [ai-playtest/](ai-playtest/), starting with
[summary.md](ai-playtest/summary.md).

### What was changed for it

| Finding | Change |
| --- | --- |
| Can't re-engage the boulder; it escapes when pushed from a terrace (s01, s02) | Pushing needs only to face the boulder roughly, from a wider arc. While pushing, Sisyphus is drawn onto the line through its centre, so he no longer slides round it. Near the boulder without pushing, the prompt says how to push. A new test pushes from a terrace, started off-centre and turned, and must reach the next terrace. |
| It rolls back silently and is lost (s01) | A message when it starts rolling back. A red HUD line while it rolls. A HUD line giving its distance and direction (ahead, left, right, behind) once it is out of reach. |
| Can't find the module named in the HUD on the stone (s01, s02) | The names of modules with issues, or rotting badly, float on the stone in their issue's colour, on the side facing you. Their glow is brighter by day. |
| The patch or retire bar lingers on other modules after release (s01, s02, s03) | Letting go of E or R starts the work over. A module at 98% or better says "Nothing to patch". |
| Terrace labels drawn through the boulder and over the HUD (s01, s02) | Labels hidden by the boulder are not drawn, and no label is drawn over the HUD. |
| What makes it heavy, and how fast each module rots, are invisible (s03) | When the boulder is heavy, the weight line says rot and modules weigh it down, and that patching or retiring lightens it. Each open issue shows how fast it is rotting, in percent a minute. |
| The release after 0.6 is 1.0, a surprise (s03) | The next-release line says when the next one is the summit. |
| F5 and F9 are listed in a session that cannot save (s02, s03) | They are left out of the controls when the game is not saving. |

Not changed: s01 felt up and down mouse looking was inverted. It matches every other game on Strata (moving the
mouse up looks up), so it was left as it is.
