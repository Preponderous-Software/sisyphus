# Playtests for 0.4.0

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
