# Playtest summary: r-sisyphus-3

3 sessions, build sisyphus-strata 0.3 (in-game release reached; build version not shown) (88acead), sisyphus-strata 0.6 reached in game (no build version shown) (88acead), sisyphus-strata 1.0 (in-game version reached; session.json version null) (88acead), generated 2026-10-28 20:59.

**Findings:** 25 reports in 25 groups -- 0 blocker, 4 major, 15 minor, 6 polish.

## Outcomes

| Session | Goal | Outcome | Fun | Minutes | Objectives | Deaths | Findings | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| s01 | Play as a brand-new player who has never heard of the game. Read the HUD and the first message, work out how to push the boulder and how to patch a module, and get as far up the hill as you can (each terrace is a release: 0.1, 0.2, ...). Note every moment of confusion, and answer the questions at the end. | achieved | 4 | 35.4 | 0 | 0 | 9 | The metaphor lands at once and the HUD messages are excellent: I understood pushing and patching from the first screen and reached 1.0 at the summit. The friction is physical: twice I slid off the boulder and walked dozens of metres past it, and finding a failing module underneath the boulder took minutes, so a direction cue to the worst issue and stickier push contact would help most. secondsToObjective is wall-clock seconds since start (there is no probe), and machine load and my two detours inflate them. |
| s02 | Upkeep with the newer mechanics: push to the second terrace (0.2), then stay there and keep the boulder alive. Patch issues as they come (hold E). When a module goes down, watch what happens to the modules that use it (the prompt says what each module uses, and red lines on the stone show the spread) and decide whether to patch the cause first. Refactor one module into the module it uses (hold F) and retire another (hold R), and say which you would rather do and why. Note when you get tired (the stamina bar) and whether sitting (Q) to rest felt worth it. Then push on as far as your time allows, and answer the questions at the end. | achieved | 3 | 28.8 | 0 | 0 | 8 | The cascade lesson works: letting parser go down turned the HUD red, made storage rot faster, and patching the one cause cleaned everything up, while users lost stayed lost. The main friction is physical: finding a named module on the ball with a centre crosshair is slow, the boulder would not turn on the 0.3 terrace, and stamina/sitting never became a real decision because the bar barely moved and sitting hides the HUD. secondsToObjective is wall-clock (no probe) and inflated by a slow, loaded machine. |
| s03 | The whole climb: get the boulder to the summit (1.0), resting (sit with Q on a terrace) when you get tired, watch what happens at the summit, and follow it back down to the trailhead. There, find the cairn raised for 1.0 beside the path, stand at it and read it, then sit and look at the view for a while. Note where the climb felt unfair, too slow or too easy, and answer the questions at the end. | partial | 3 | 38.4 | 0 | 0 | 8 | The premise lands immediately: the opening text, 'It will not last.', the sit view and the rolling-back message are evocative, and the early climb (0.0-0.4) feels good. What broke the run was not the push but finding modules: open issues aren't marked on the boulder at patch range, a module at the contact point can't be reached, and once 2+ modules are down the HUD list overprints the crosshair prompt so you can't even read what you're aiming at; I reached 0.6, got to 5 m below the summit, and rolled back. secondsToObjective is wall-clock (no probe), and machine load (32 s game took ~3.5 min real) meant I never reached 1.0 or the cairn. |

## Answers

Each agent's own words, verbatim from its report; a missing answer is listed under problems.

### Q1. In your own words, what did you think the game was about? What did the boulder, the terraces and the rot stand for?

- **s01:** Maintaining software forever. The boulder is the codebase (its facets are modules like core, auth, router, search, billing), each terrace is a release that brings more users and one more module, so the thing you push gets bigger and heavier. Rot is entropy: flaky tests, expiring certificates, deprecated APIs, dependency drift, security advisories, all things that go wrong in code nobody touches. The summit message about the platform reaching end of life, with the boulder rolling back, says the work is never finished, which is the Sisyphus joke.
- **s02:** It is about maintaining software forever: the boulder is the codebase (each coloured patch a module), each terrace is a release that brings more users and one more module to carry, and rot is the decay that happens to code you are not touching - dependency drift, deprecated APIs. The work never ends; growth only adds more to keep alive, and the honest moves are patching, merging (refactor) or cutting (retire).
- **s03:** Maintaining software forever. The boulder is the codebase (every face a module, labelled ui, core, auth, billing...), each terrace is a shipped release that adds a module and users, and the rot is ordinary decay: dependency drift, deprecated APIs, flaky tests, expiring certificates, security advisories. The weight bar says unmaintained code makes every next release heavier, and the rollback says if you neglect it the project slides back. Sitting on a terrace felt like the moment of rest between releases.

### Q2. Did the rot and the upkeep feel fair, or arbitrary? What was hardest to tell or to do?

- **s01:** Mostly fair. The issue list showed the module, the cause, the health % and the decay rate, so I could tell how urgent each one was, and letting router hit 0% had a clear, logical consequence (outage, users leaving, the module that uses it rotting faster). The hardest part was physical, not the rules: finding the rotting module on the boulder (it was often underneath or on the far side, and it rots while you are on the slope where you cannot stop to patch), and keeping contact while pushing. I slid past the boulder twice. The decay after the summit, while I could do nothing, felt less fair.
- **s02:** Mostly fair: rates are shown per minute, a down module clearly explains its effect on its users, and fixing the cause cleared the cascade at once. Hardest was physically reaching the module in the issues list on the ball (storage on 0.3 I never reached) and telling 'rotting' in a prompt apart from an open issue. Between refactor and retire I would rather refactor: it cut a module (8->7) without losing users and left the merged module fresh at 100%, whereas retiring search cost 7 users. Stamina never made me tired enough to matter, and sitting hid the HUD so I could not judge if resting was worth it.
- **s03:** The rules felt fair and readable in the HUD (each issue has a rate, the dependency 'uses X' chain explains cascades, 'rot weighs it down' explains the slowdown). What felt arbitrary was where the rot landed: a module on the underside or at the contact point can't be reached, and nothing on the boulder up close tells you which face is the sick one. Hardest by far was finding the module named in the issue list; second was reading the crosshair prompt once the issue list grew into it. I lost the run (rolled back from 5 m below 1.0) to the finding problem, not to the push.

## Findings

### 1. [major] A module at the boulder's contact point can't be reached and rots to DOWN, triggering an unrecoverable cascade

- **Category:** balance; **reported by** 1 agent(s): s03
- **Detail:** Billing got dependency drift while on the slope (-35%/min). On every terrace afterwards it sat at the bottom of the boulder (orange glow at the ground contact). Pushing doesn't bring it round reliably, and rolling it back from the uphill side needs precise positioning. It went DOWN, users started leaving, search/auth followed, router reached -101%/min, weight maxed and at 5 m below 1.0 the HUD said 'too heavy to push uphill' and the boulder rolled 23 m back to the 0.6 terrace. Once two modules are down the rest rot faster than one person can patch, so it felt unfair rather than a choice.
- **Repro:** pt start ... --seed 3; push straight up to 0.6 patching only what is visible; leave billing (it appears on the slope from 0.3 to 0.4)
- **Screenshots:** [spiral.png](s03/shots/spiral.png)

### 2. [major] Holding W from behind the boulder often slides Sisyphus past it instead of pushing

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** Twice on the 0.2 terrace I lined up behind the boulder (third-person V view, boulder filling the right 2/3 of the screen) and held Shift+W. The first time the boulder did not move and he walked 49 m on up the path; the second time it moved 3 m (33 -> 30 m) and then he walked past it, ending 32 m above. There is no feedback that contact was lost. Expected the push to keep him on the boulder, or a warning like 'you lost the boulder'. Once the boulder was dead-centre on the crosshair, pushing was reliable.
- **Repro:** pt start --profile sisyphus-strata --seed 1; push to 0.2; walk around to the downhill side so the boulder is slightly right of centre; pt hold Shift_L+w 12; pt shot -> 'The boulder: 32 m behind you'
- **Screenshots:** [slipoff.png](s01/shots/slipoff.png)

### 3. [major] No way to locate an open module on the boulder up close; hunting faces eats minutes

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** Open issues are listed in the HUD but the boulder only shows module name labels when you stand ~5+ m away; up close (patch range ~3 m) nothing marks the rotting face. Colours repeat across modules (purple was 'router' at 0.2, 'cache' at 0.4) and the dark faces (search, billing, storage) look alike. At 0.2 it took ~5 real minutes of walking round the boulder to find 'search' on its underside; at 0.4 I gave up on billing. Expected: a marker/arrow on the issued face, or the label visible at patch range, or a 'nearest open issue: turn left' hint like the existing 'The boulder: 6 m to your right'.
- **Repro:** pt start --profile sisyphus-strata --seed 3; pt hold w 45 (ship 0.1, 0.2); on terrace 0.2 try to find 'search' from the HUD issue list
- **Screenshots:** [find_search.png](s03/shots/find_search.png)

### 4. [major] With 5+ open issues the HUD issue list overprints the centre crosshair prompt

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** The issue list grows down into the screen centre where the module prompt and messages are drawn. At 5 issues 'On the slope it rolls back...' is printed over the uploads row; at 8-9 issues the module prompt (name/health/'Hold E to patch') is unreadable, so I could not tell what I was aiming at and patched ui by accident when trying to reach a downed module. This hits exactly when the player most needs to read it.
- **Repro:** Let issues accumulate past 5 (seed 3, around 0.6) and look at the boulder
- **Screenshots:** [overlap.png](s03/shots/overlap.png)

### 5. [minor] 'Next release ... (behind you/to your right)' line is cut off at the right edge

- **Category:** text; **reported by** 1 agent(s): s03
- **Detail:** 'Next release 1.0 (the summit): 34 m uphill (behind y' and '(to your' are truncated at 640 px width.
- **Repro:** Reach 0.6, turn so the summit is behind or to the side
- **Screenshots:** [sit2.png](s03/shots/sit2.png)

### 6. [minor] 'rotting (NN%)' in a module's prompt is not an open issue, and nothing says so

- **Category:** text; **reported by** 1 agent(s): s02
- **Detail:** billing showed 'rotting (86%)' and ui 'rotting (75%)' in their prompts while neither appeared under OPEN ISSUES. Meanwhile modules under 'dependency drift' were listed. Patching core (used by several modules) while it was only drifting did nothing for the others' drift. I could not tell whether rotting modules need patching or what puts a module on the issues list.
- **Repro:** pt start ... --seed 2; reach 0.2; aim at billing (pt look) and compare its prompt with the OPEN ISSUES list
- **Screenshots:** [down1.png](s02/shots/down1.png)

### 7. [minor] After the summit, users keep leaving and modules keep rotting while the player can do nothing

- **Category:** balance; **reported by** 1 agent(s): s01
- **Detail:** 1.0 shipped and the boulder rolled away ('It rolls. Follow it down.'), but the router outage kept draining users (368 -> 314 in ~16 s) and modules kept rotting. If that is intended (the next climb inherits the mess), the screen should say so; otherwise it feels like a punishment after the payoff.
- **Repro:** reach 1.0 with a module DOWN; pt wait 10; pt shot
- **Screenshots:** [after2.png](s01/shots/after2.png)

### 8. [minor] Finding the module named in OPEN ISSUES on the boulder is slow and sometimes impossible

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** On the 0.3 terrace storage was the worst issue (41%->37%) and I could not get the crosshair on it in ~3 minutes. From the uphill side, walking into the boulder (W) no longer rolled it (it just stays put, presumably at the terrace edge, with no message), and circling a ball with a fixed centre crosshair kept landing on auth/ui/core. Nothing on the stone or in the HUD points to where a listed module is. On 0.2 finding router also took ~8 look/walk steps. Expected: some pointer (an on-stone highlight for the module under the worst issue, or 'storage: underneath/far side' text) or a reliable way to turn the boulder on a terrace.
- **Repro:** pt start --profile sisyphus-strata --seed 2; push to 0.3 (Shift+W ~36 s from 0.2); walk to the uphill side; pt hold w 1.5 into the boulder; pt look in steps of 150-300 px; storage never comes under the crosshair
- **Screenshots:** [rb8.png](s02/shots/rb8.png)

### 9. [minor] HUD and big issue list cover the top-left/centre of the view so the boulder is hidden behind text

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** At 640x360 with 9 issues the HUD takes ~70% of the screen height; the boulder on the terrace was behind the text and I had to guess where to aim. A compact mode or collapsing the list to the worst 3 would help.
- **Repro:** As above, at 0.6 with 9 issues
- **Screenshots:** [r31.png](s03/shots/r31.png)

### 10. [minor] Mouse-look turn amount felt inconsistent for the same mouse motion

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** pt look 1000 0 turned about 90 degrees once and about 180 degrees another time; pt look 500 0 sometimes about 45, sometimes about 90 (judged by the HUD 'to your left/right/behind you' cues). Made it hard to turn around reliably to find the boulder. It may be the harness's relative motion, not the game.
- **Repro:** pt look 1000 0; pt step 2; pt shot; repeat and compare the HUD direction cues
- **Screenshots:** [back2.png](s01/shots/back2.png)

### 11. [minor] No way to tell where a failing module is on the boulder from a distance or underneath

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** The first issue (search, flaky test) appeared while pushing; on the 0.2 terrace it took several minutes of looking around and walking to find it, because it was underneath the boulder. Only a faint floating label shows from some angles. The hint 'From up here, walking into it rolls it back' helps but a direction cue to the worst issue (like the 'to your left' cue for the next release) would help a lot.
- **Repro:** pt start ... --seed 1; push to 0.1 and 0.2; try to find 'search' on the boulder
- **Screenshots:** [patched.png](s01/shots/patched.png)

### 12. [minor] Push speed drops sharply with weight; last 30 m to 1.0 took over 60 s of pushing

- **Category:** balance; **reported by** 1 agent(s): s03
- **Detail:** Early terraces: ~1 m/s (33 m in ~27 s). At 0.6 with 'heavy' weight: 3 m in 4 s, then 6 m in 25 s, then rollback. The feedback is clear ('rot weighs it down: patch, retire or refactor'), which is good, but combined with the finding problem the slowdown snowballs.
- **Repro:** seed 3, push from 0.6 toward 1.0 with 4+ issues
- **Screenshots:** [r26.png](s03/shots/r26.png)

### 13. [minor] Red spread lines on the stone were not noticeable

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** When parser went down and storage started rotting faster ('uses parser, down', -45%/min), the HUD made the cascade clear, but I did not notice red lines on the stone from either view; only a few red glints near the bottom edge (fp2). The HUD did the job; the stone did not.
- **Repro:** pt start ... --seed 2; reach 0.2; leave parser unpatched until DOWN; pt key v; pt shot
- **Screenshots:** [fp2.png](s02/shots/fp2.png)

### 14. [minor] Rolling the boulder back on a terrace shows 'The boulder is rolling back down!' and the target module slips away mid-patch

- **Category:** bug; **reported by** 1 agent(s): s01
- **Detail:** On the 0.2 terrace I pushed from the uphill side for 1.5 s as the hint suggests. The HUD showed red 'The boulder is rolling back down!' (alarming; it actually stopped at the edge, 32 -> 34 m). While it was still turning I held E on search, but the crosshair drifted to auth, so the patch went to auth instead.
- **Repro:** on 0.2 terrace, stand uphill of the boulder facing it; pt hold w 1.5; pt hold e 3
- **Screenshots:** [rollback.png](s01/shots/rollback.png)

### 15. [minor] Sitting (Q) hides the entire HUD, so rest cannot be judged

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** Q shows a lovely still view with 'The boulder is still. For a moment, so are you.', but the HUD (stamina, issues, users) disappears, so I could not see stamina refill or whether modules rotted meanwhile. Getting up also left me facing downhill away from the boulder. Expected: at least a small stamina bar while sitting.
- **Repro:** pt start ...; reach 0.3; pt key q; pt wait 5; pt shot
- **Screenshots:** [sit1.png](s02/shots/sit1.png)

### 16. [minor] Sitting (Q) hides the whole HUD, so a module can go DOWN unseen while resting

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** Sitting shows a lovely 'The boulder is still. For a moment, so are you.' view, but no stamina or issues. I sat ~16 s at 0.6 with billing at 8% and stood up to an outage. Expected at least a minimal stamina/alert line, or a nudge before a module goes down.
- **Repro:** On a terrace with an issue below 20%, pt key q; pt wait 16; pt hold s 0.2
- **Screenshots:** [sit1.png](s03/shots/sit1.png)

### 17. [minor] Stamina bar barely matters and its visibility is unclear

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** The Stamina bar appeared on the 0.1->0.2 slope (~90%), was visible during a 10 s refactor without dropping, then disappeared from the HUD for the whole 36 s Shift+W push from 0.2 to 0.3 (push1-push3) and reappeared at 90% on arrival. I was never tired enough to be slowed or to need rest. Expected: the bar visible whenever it is changing, and pushing/hurrying drawing it down enough to make resting a decision.
- **Repro:** pt start ... --seed 2; push to 0.2; pt hold Shift_L+w 12 (x3) toward 0.3; screenshot each
- **Screenshots:** [push4.png](s02/shots/push4.png)

### 18. [minor] Stepped headless session runs far slower than real time under load

- **Category:** perf; **reported by** 1 agent(s): s02
- **Detail:** A 15 s 'pt hold w' took about 5 minutes of wall-clock, and 'pt wait 70' about 4 minutes, which limited how much upkeep fit into 45 minutes. Probably machine load from parallel sessions rather than the game.
- **Repro:** pt start ... --seed 2; time pt hold w 15
- **Screenshots:** [p4.png](s02/shots/p4.png)

### 19. [minor] The last metre before each terrace takes extra pushing (stalls at '1 m uphill')

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** At 0.2, 0.3 and 0.6 a long Shift+W push ended on '1 m uphill' and needed another short W press to ship. Not a big deal but it felt like a stall right before the reward.
- **Repro:** push toward any terrace with pt hold Shift_L+w N; observe 'Next release ...: 1 m uphill'
- **Screenshots:** [ship06.png](s01/shots/ship06.png)

### 20. [polish] Floating module label is drawn under the crosshair prompt text and becomes unreadable

- **Category:** text; **reported by** 1 agent(s): s01
- **Detail:** On the 0.4 terrace the 'billing' label on the boulder sat in the lower middle of the screen and was overprinted by the prompt '(hold R to retire it, F to refactor...)'.
- **Repro:** ship 0.4; look slightly down at the boulder's lower half in V view
- **Screenshots:** [overlap.png](s01/shots/overlap.png)

### 21. [polish] Good: outage and fixing the cause read clearly

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** When parser went down the HUD turned red ('OUTAGE: users are leaving', users 100->74) and storage was labelled 'uses parser, down' at a faster rot. Patching parser alone cleared storage's issue, the outage, and dropped weight 7->4. That cause-first lesson landed well. Lost users did not return, which made the outage cost felt.
- **Repro:** as above; aim at parser; pt hold e 5
- **Screenshots:** [down1.png](s02/shots/down1.png)

### 22. [polish] Good: the opening message, ship messages and outage warnings are clear

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** The first message ('The boulder is your software. Push it up... Hold E on one to patch it.') explained the loop immediately. 'Shipped 0.x. New module: ...', the red 'OUTAGE: users are leaving', 'uses router, down', 'tired: sit down (Q) to rest' and 'rot weighs it down' all told me exactly what was going on. The summit text 'The platform it runs on reached its end of life' is a great beat.
- **Repro:** pt start ... --seed 1; play to 1.0
- **Screenshots:** [outage.png](s01/shots/outage.png)

### 23. [polish] Holding E on a DOWN module for 3 s shows no progress bar at all

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** On parser (DOWN 0%), a 3 s E hold left the prompt unchanged with no bar (pp1); a further 4 s hold completed the patch (pp2). Either down modules take longer with no visible bar, or progress is lost on release silently. Expected: a progress bar from the first frame, as with refactor ('Refactoring it into auth [##...]').
- **Repro:** pt start ... --seed 2; reach 0.2; let parser go down (~3 game-min); aim at parser; pt hold e 3; pt shot
- **Screenshots:** [pp2.png](s02/shots/pp2.png)

### 24. [polish] Prompt says 'Hold E to patch' on a healthy module; issue reads 'flaky test +' with an unexplained '+'

- **Category:** text; **reported by** 1 agent(s): s01
- **Detail:** Modules at 94-96% that are labelled 'healthy' still say 'Hold E to patch'. After the summit, parser's issue showed 'flaky test +' and I could not tell what the '+' means.
- **Repro:** look at any 95% module; and see after2.png
- **Screenshots:** [after2.png](s01/shots/after2.png)

### 25. [polish] Very slow wall-clock under load: 32 s of game pushing took ~3.5 min real

- **Category:** perf; **reported by** 1 agent(s): s03
- **Detail:** With parallel sessions, pt hold w 32 ran for ~210 s wall-clock. Not a game bug as such, but the climb is long in game seconds and this made the 55 min budget too short to reach 1.0 and the cairn.
- **Repro:** pt hold w 32 with other sessions running
- **Screenshots:** [r15.png](s03/shots/r15.png)

## Objective timings

Game seconds from the start of the world to each objective (event tick / the game's ticks per second, from events.jsonl). Game time, not wall time: with many sessions on one machine a world can run slower than real time, and wall-clock timings would mostly measure the machine.

No objective was completed (or the game has no probe).

