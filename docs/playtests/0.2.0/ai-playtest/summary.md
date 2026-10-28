# Playtest summary: r-sisyphus-2

3 sessions, build sisyphus-strata 0.1 (in-game release reached; session.json version null) (8ce3be2), sisyphus-strata 0.2 (in-game version reached; build version not shown) (8ce3be2), sisyphus-strata 0.5 (in-game version reached); session.json version null (8ce3be2), generated 2026-10-28 07:55.

**Findings:** 21 reports in 18 groups -- 0 blocker, 4 major, 10 minor, 7 polish.

## Outcomes

| Session | Goal | Outcome | Fun | Minutes | Objectives | Deaths | Findings | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| s01 | Play as a brand-new player who has never heard of the game. Read the HUD and the first message, work out how to push the boulder and how to patch a module, and get as far up the hill as you can (each terrace is a release: 0.1, 0.2, ...). Note every moment of confusion, and answer the questions at the end. | partial | 2 | 27.2 | 0 | 0 | 6 | The premise lands immediately and the first climb to 0.1 plus the patch feedback are lovely, but after 0.1 I never moved the boulder more than a metre again: the walker slides off it, and stopping on the slope to patch rolls it back without warning, so 30 minutes ended at 0.1. Fix the push contact and tell the player upkeep happens on terraces, and the loop would work. secondsToObjective are wall-clock seconds (no probe). |
| s02 | Upkeep: push to the second terrace (0.2), then stay there a while and keep the boulder alive: patch modules as issues arrive (hold E), roll the boulder back from above to reach a module underneath, and retire one module (hold R). Say whether you could always tell which module was in trouble and where it was. Then push on as far as your time allows, and answer the questions at the end. | achieved | 3 | 29.4 | 0 | 0 | 6 | The idea comes through clearly: the HUD ('No open issues. It will not last.'), the release flags, users growing per release, glowing module labels coloured by severity and 'Retired... a few users miss it' all make the upkeep metaphor clear and a bit poignant. Most of my time went on the physical controls, not the upkeep: the boulder slides off unless you are lined up exactly, and modules on top need first-person aiming. Upkeep was easy once on a terrace (health never fell below ~85% at 0.2), so the challenge was the camera rather than the rot. secondsToObjective are wall-clock seconds from pt start (no probe). |
| s03 | The whole climb: get the boulder to the summit (1.0), watch what happens there, follow it back down to the trailhead, and then sit (Q) and look at the view for a while. Note where the climb felt unfair, too slow or too easy, and answer the questions at the end. | partial | 3 | 42.1 | 0 | 0 | 9 | The theme reads instantly and the writing ("It will not last.", sitting with the still boulder) is strong; the first three releases feel good. From 0.4 on, rot spawns faster than issues can be found and patched, the boulder becomes 'too heavy to push uphill' at 0.5, and getting back below the boulder after a rollback is the single biggest time sink because nothing shows which way is uphill. secondsToObjective are rough wall-clock estimates (no probe); I reached 0.5 and sat, but never the summit. |

## Answers

Each agent's own words, verbatim from its report; a missing answer is listed under problems.

### Q1. In your own words, what did you think the game was about? What did the boulder, the terraces and the rot stand for?

- **s01:** Maintaining software forever. The boulder is the codebase itself, the coloured patches are its modules (auth, parser, storage, search, core, router, ui). Each terrace is a release: getting there ships a version, brings users and bolts a new module on, which makes the boulder heavier. The rot is the ordinary decay of software: flaky tests, expiring certificates, deprecated APIs, each bleeding health per minute. Letting go makes it roll back, i.e. you lose ground if you stop maintaining; and 'It will not last' says the work never ends.
- **s02:** Maintaining software that people depend on, and how that work never ends. The boulder is the codebase: the modules (auth, router, storage, parser, ui, core, search, billing) are patches on its surface. Each terrace is a shipped release: it brings more users (10->50->100) and one more module, so more weight. The rot is entropy from outside: dependency drift, deprecated APIs, expiring certificates and security advisories turn up even when you don't touch anything ('No open issues. It will not last.'). Stopping to fix things on the slope lets it roll back, as if maintenance work kept stalling feature progress. Retiring a module lightens the load at a small cost in users.
- **s03:** Maintaining software forever: the boulder is the product you keep pushing, each terrace is a shipped release that wins users but bolts on another module, and the rot (dependency drift, deprecated APIs, security advisories) is the constant entropy that turns every new feature into more upkeep. The bigger it gets, the heavier it is to move; stop pushing to fix something and it slides back. I never reached the summit, but the 'It will not last' line and sitting beside the still boulder suggest the point is accepting the endless upkeep rather than finishing it.

### Q2. Did the rot and the upkeep feel fair, or arbitrary? What was hardest to tell or to do?

- **s01:** The rot itself felt fair: issues are named, show a rate (-30%/min, -60%/min), and patching takes 1-5 s and works. What felt arbitrary was the reach: rotting modules were often on the underside, and stopping to patch on the slope rolled the boulder back, which I only learned by losing progress. Hardest of all was physically getting below the boulder and keeping it in front of me; after 0.1 I could not push it up again, so I could not tell whether the balance of rot vs. climbing is right.
- **s02:** The rot felt fair. Issues come one at a time, the HUD lists them worst first with a %/min rate, and the label of a module with an issue glows on the boulder (red for the security advisory), so I always knew which module was in trouble. Security issues rot about twice as fast as drift, which makes sense. On the 0.2 terrace it was if anything gentle: overall health stayed at 85% or above while I patched seven issues. Where the module was came across less well: the HUD names it but doesn't say which side it's on, and labels on the far side, underneath or on top can't be seen until you walk round. A module on top could only be targeted in first person (V). The hardest part by far was the physical push: keeping the boulder lined up so I didn't slide round it, and keeping track of which way was uphill. Rolling it back from above to turn it worked and the 'Rolling it back to turn it' message helped.
- **s03:** Fair up to about 0.3: issues were few, the HUD rates (-35%/min drift vs -72%/min advisory) made priorities clear, and glowing labels on the boulder showed where they were. From 0.4 it felt overwhelming rather than fair: new issues appeared faster than I could find, face and patch them, two modules went DOWN and the weight maxed out at 0.5. Hardest to do was getting back below the boulder after it rolled back (no sense of which way was uphill), and reaching issues on the underside; hardest to tell was that patching mid-slope would let the boulder roll back, and whether retiring a module was worth it.

## Findings

### 1. [major] Cannot push the boulder off the 0.1 terrace: walker slides past it on one side

- **Category:** bug; **reported by** 1 agent(s): s01
- **Detail:** From 0.0 to 0.1, pushing was effortless (W then Shift+W, ~29 s). From the 0.1 terrace on, seven separate attempts from below (third person and first person, plain W and Shift+W, boulder centred on crosshair or on the character, and alternating 'hold w 0.6' with 'look -25 0' to steer into it) each gained at most 1 m (28 -> 27 -> 26 m) before the walker slid off the boulder's right side and kept walking uphill alone ('The boulder: 5-20 m behind you'). The boulder then rolls back. Expected: walking into the boulder from below to catch and push it as it did on the first slope.
- **Repro:** pt start --profile sisyphus-strata --game-dir <sis-pt> --seed 1; pt hold Shift_L+w 29 (ships 0.1); stop on slope so it rolls back to the 0.1 terrace; walk below it on the path, face it, pt hold Shift_L+w 4 -> walker passes it on the right, 'Next release 0.2' stays ~27 m
- **Screenshots:** [lostcontact.png](s01/shots/lostcontact.png)

### 2. [major] Pushing the boulder up the slope keeps sliding Sisyphus around it instead of pushing

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** Unless the boulder is lined up with the character's body (not the crosshair, which sits to the right in the over-shoulder view), holding W deflects him around the boulder and past it; the 'Next release' distance barely moves (29->28 m) and the boulder ends up 'behind you'. About 12 of my first 17 minutes went on re-finding, circling and re-aligning. Once lined up (boulder centre roughly on the character, slightly right), pushing was smooth at ~1 m/s. Expected either a little auto-centering/magnetism while pushing, or a cue showing the push contact.
- **Repro:** pt start --profile sisyphus-strata --seed 2; get downhill of the boulder with it ~1 character-width off-centre; pt hold w 5; pt shot
- **Screenshots:** [slide-past.png](s02/shots/slide-past.png)

### 3. [major] Re-positioning below the boulder after a rollback is very hard: no uphill/path cue

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** After the boulder rolled back to the 0.3 terrace it took ~10 minutes wall clock to get below it and push it up again. Holding W toward it repeatedly took me past it or up the valley wall ('The boulder: 25 m behind you') while 'Next release' stayed at 28 m; the third-person camera loses the boulder when you stand beside it, and nothing shows which way the path goes uphill. The 'From up here, walking into it rolls it back' hint helps, but only once you are already on the wrong side. Expected a direction cue (arrow to the push side, or a path marker).
- **Repro:** pt start ... --seed 3; push to 0.3; let the boulder roll back; try to get below it and push again
- **Screenshots:** [lost.png](s03/shots/lost.png)

### 4. [major] Rot outpaces upkeep after 0.4: weight maxes out at 0.5 and the boulder can no longer be pushed

- **Category:** balance; **reported by** 1 agent(s): s03
- **Detail:** Between 0.4 and 0.5 five new issues appeared (security advisories at -72%/min). By 0.5 there were 9 open issues, router and ui DOWN, 'OUTAGE: users are leaving', and the Weight bar full with 'too heavy to push uphill'. Holding W for 25 s game time moved the boulder backward (15 m -> 18 m) and it then rolled back to the 0.5 terrace. Each patch costs 3-5 s plus finding the module, so one player cannot keep up; the climb stalled at 0.5 of 1.0. Expected the second half to be hard, but reachable with diligent patching.
- **Repro:** pt start --profile sisyphus-strata --seed 3; push (hold w) terrace to terrace, patching on each terrace; observe issue count and Weight from 0.4 onward
- **Screenshots:** [tooheavy.png](s03/shots/tooheavy.png)

### 5. [minor] No warning that letting go on the slope rolls the boulder back to the last terrace

- **Category:** ux; **reported by** 3 agent(s): s01, s02, s03
- **Also reported as:** "Patch prompt offered mid-slope; holding E lets the boulder roll back a whole terrace"; "Stopping to patch on the slope lets the boulder roll back to the terrace below; the hint then talks about level ground"
- **Detail:** The opening text says push W and hold E to patch, which invites stopping mid-slope to patch the first issue (search: flaky test) that appears 19 m up. Holding E there rolled the boulder back to the 0.1 terrace, losing ~12 m. Later a patch on the slope was interrupted the same way (router stayed at 78%). The rule 'you can only patch on a terrace' is learned only by losing progress. Expected: a line in the intro or when the first issue appears ('patch it on the next terrace; let go on the slope and it rolls back').
- **Repro:** pt start --profile sisyphus-strata --game-dir <sis-pt> --seed 1; pt hold w 5; pt hold Shift_L+w 24 (ship 0.1); pt hold Shift_L+w 15 (issue appears); pt hold e 2 -> 'The boulder is rolling back down!'
- **Screenshots:** [rollback.png](s01/shots/rollback.png) [rollback-hint.png](s02/shots/rollback-hint.png) [rollback.png](s03/shots/rollback.png)

### 6. [minor] Rotting modules on the boulder's underside are out of reach and not all are tinted

- **Category:** ux; **reported by** 2 agent(s): s01, s03
- **Also reported as:** "Issues on the boulder's underside can be seen but not targeted"
- **Detail:** 'search' (flaky test) and 'router' (deprecated API) both sat on the underside; their in-world labels only showed from some angles and the purple glow was the only clue. 'auth' turned a bright yellow-green when rotting, but 'core' (deprecated API, 60%) kept its normal dark blue so it was hard to spot. Rolling it from the uphill side to bring them up is mentioned in the controls notes but not in the game.
- **Repro:** pt start --profile sisyphus-strata --game-dir <sis-pt> --seed 1; ship 0.1; wait for 'search' issue; try to find it on the boulder
- **Screenshots:** [coreok.png](s01/shots/coreok.png) [underside.png](s03/shots/underside.png)

### 7. [minor] Hard to tell which way is uphill on the 0.1 terrace; much time lost circling the boulder

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** After the roll-back I spent ~15 minutes trying to get below the boulder. 'The boulder: N m ahead/to your right/behind you' in the HUD is very helpful, but nothing points uphill or along the path, the 'From up here...' / 'Face the boulder...' hints flip as you circle it, and the path is hard to read on the software renderer. Expected: an uphill/path arrow, or the hint saying which side to go to.
- **Repro:** pt start --profile sisyphus-strata --game-dir <sis-pt> --seed 1; ship 0.1, let it roll back, then try to get below it using screenshots only
- **Screenshots:** [orient.png](s01/shots/orient.png)

### 8. [minor] Holding R after a retire goes straight on to retiring the next module under the crosshair

- **Category:** bug; **reported by** 1 agent(s): s02
- **Detail:** I held R on router: it retired (Users 100->93, Modules 8->7, 'Retired. One less thing to keep, and a few users miss it.'). The same held R then started 'Retiring [####......]' on storage, a healthy (99%) module that was now under the crosshair. Releasing in time saved it, but a player holding R a bit longer would lose a second module without meaning to. Expected R to need a fresh press after each retire, as a destructive action should.
- **Repro:** on a terrace, aim at a module; pt hold r 1.5; pt hold r 4; pt shot (Retiring bar on the next module)
- **Screenshots:** [retire-chain.png](s02/shots/retire-chain.png)

### 9. [minor] Modules on top of the boulder can't be targeted from the over-shoulder camera

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** Router (security advisory, red label, -72%/min) sat on top of the boulder. With the default camera, pitching the view moved the camera but the crosshair stayed on the boulder's side, from both the uphill and downhill sides. I had to press V for first person and look up to reach it. Expected the over-shoulder crosshair to reach the top of the boulder, or a hint to switch to V.
- **Repro:** on the 0.2 terrace with a module issue on top of the boulder: walk up to the boulder; pt look 0 -40; pt shot (crosshair still on a side module); pt key v; pt look 90 -40
- **Screenshots:** [top-reach.png](s02/shots/top-reach.png)

### 10. [minor] Patch/retire prompt does not name the face under the crosshair

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** In first person next to the boulder, the prompt named 'storage' over both a dark-green and a gold face, and named 'auth: healthy (98%) Hold E to patch' while the crosshair was on open ground beside the boulder. The target seems to be picked by proximity, not by the crosshair, so aiming at the floating 'search' label is the only reliable way. Expected: the prompt follows the crosshair (or highlights the targeted module).
- **Repro:** pt start --profile sisyphus-strata --game-dir <sis-pt> --seed 1; push to 0.1; pt key v; near the boulder pt look 40 0 / -80 0 and read the prompt under the crosshair
- **Screenshots:** [target.png](s01/shots/target.png)

### 11. [minor] Weight hint cut off at the right edge during an outage

- **Category:** text; **reported by** 1 agent(s): s03
- **Detail:** With weight 'heavy' the HUD line '(rot and modules weigh it down: patch, or retire,' runs off the right edge of the 640x360 window; the rest of the sentence is never visible.
- **Repro:** pt start ... --seed 3; let a module reach 0% (DOWN) around 0.5
- **Screenshots:** [outage.png](s03/shots/outage.png)

### 12. [polish] Could not see the patch progress bar in screenshots after a partial hold

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** After a 1-1.5 s hold of E on storage/auth, the next screenshot showed only the static 'Hold E to patch' prompt with no partial bar or kept progress. The bar may only draw while the key is down (the harness releases before the shot), but if partial progress is lost on release, a player who lets go early gets no credit and no sign of it. Patches finished in about 2-5 s of holding. The Retiring bar did show (shots/retire-chain.png).
- **Repro:** aim at a rotting module; pt hold e 1.5; pt shot
- **Screenshots:** [nobar.png](s02/shots/nobar.png)

### 13. [polish] Good: opening screen, release toast and patch feedback are clear

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** The first screen explains the whole idea in two lines; 'Shipped 0.1. New module: search.' with users 10->50 feels good; patching shows a progress bar and 'auth: healthy (100%)', and 'No open issues. It will not last.' sets the tone. Recorded so it is kept.
- **Repro:** pt start --profile sisyphus-strata --game-dir <sis-pt> --seed 1; pt shot open; pt hold Shift_L+w 29
- **Screenshots:** [ship01.png](s01/shots/ship01.png)

### 14. [polish] Good: sitting (Q) is a calm, well-judged beat

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** Sitting hides the HUD and shows 'The boulder is still. For a moment, so are you.' over the valley, even mid-outage. Opening text 'No open issues. It will not last.' and the 'Shipped 0.x. New module: ...' toasts also land well.
- **Repro:** pt start ... --seed 3; on any terrace, pt key q; pt wait 5
- **Screenshots:** [sit2.png](s03/shots/sit2.png)

### 15. [polish] Late-game holds run far slower than real time on the shared software renderer

- **Category:** perf; **reported by** 1 agent(s): s03
- **Detail:** Near 0.5 (11 modules, many glowing issues) 'pt hold w 25' took over 2 minutes of wall clock; earlier 20 s holds took well under a minute. Likely machine load from parallel sessions, but it made the outage stretch very slow to play through.
- **Repro:** pt start ... --seed 3; at 0.5 with 9 open issues, pt hold w 25
- **Screenshots:** [tooheavy.png](s03/shots/tooheavy.png)

### 16. [polish] Long issue list and in-world labels overlap the crosshair, prompt and help bar

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** With 4+ open issues the issue list reaches the crosshair row and with 7+ the '... and N more' line collides with the patch prompt; module labels (e.g. 'billing') draw over the help bar and prompt text.
- **Repro:** pt start ... --seed 3; reach 0.3-0.5 with several open issues; V and aim at the boulder
- **Screenshots:** [overlap.png](s03/shots/overlap.png)

### 17. [polish] Shift (hurry) does not speed up pushing

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** pt hold w 5 moved the boulder 6 m; pt hold Shift_L+w 5 also moved it 6 m. The help bar lists 'Shift hurry' next to walk/push, so I expected faster pushing (or a note that hurry only affects walking).
- **Repro:** pt start ... --seed 3; pt hold w 5; pt hold Shift_L+w 5; compare 'Next release' distance
- **Screenshots:** [p2.png](s03/shots/p2.png)

### 18. [polish] Terrace flag label draws over the module prompt in first person

- **Category:** text; **reported by** 1 agent(s): s02
- **Detail:** In V (first person) the '0.2' flag's label was drawn on top of the module prompt: 'route0.2security advisory, a vulnerability, and it spreads'. Expected the HUD prompt to draw above world labels. Also, two module labels on the boulder (storage/parser) overlapped each other in over-shoulder view (shots/c1.png).
- **Repro:** on the 0.2 terrace, pt key v, aim at a module with the 0.2 flag behind it
- **Screenshots:** [overlap.png](s02/shots/overlap.png)

## Objective timings

Game seconds from the start of the world to each objective (event tick / the game's ticks per second, from events.jsonl). Game time, not wall time: with many sessions on one machine a world can run slower than real time, and wall-clock timings would mostly measure the machine.

No objective was completed (or the game has no probe).

