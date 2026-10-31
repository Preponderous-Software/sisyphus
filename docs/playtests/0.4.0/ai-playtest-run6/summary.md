# Playtest summary: r-sisyphus-6

4 sessions, build sisyphus-strata 0.2 (in-game, climbed to 0.4) (9a68cbf), sisyphus-strata 0.5 reached in game (HUD shows release, not build); session.json version null (9a68cbf), sisyphus-strata 1.1 (in-game software version at start; game build version not shown) (9a68cbf), sisyphus-strata 1.1 (in-game; session.json version null) (9a68cbf), generated 2026-10-30 18:44.

**Findings:** 28 reports in 24 groups -- 0 blocker, 2 major, 14 minor, 12 polish.

## Outcomes

| Session | Goal | Outcome | Fun | Minutes | Objectives | Deaths | Findings | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| s01 | You start near the top of the hill (version 0.6), with the boulder resting on the terrace below the summit. Push it up to the summit and ship 1.0. While the summit pause lasts, the game offers a choice: let it roll back down with every module aboard, or hold R to rewrite it. Work out the choice from the screen, choose the rewrite, follow it down, and play on from the trailhead. Then open the History screen (Tab) and judge whether the rewrite was clear and felt like a real decision. Note every moment of confusion. | achieved | 3 | 20.1 | 0 | 0 | 7 | The rewrite choice itself is well staged: the summit text names both options and their costs, the hold-R bar is responsive, and the trailhead payoff ('6 fresh modules, most users did not follow', 500->200 users) lands. It is undercut by the roll-down banner that says 'every module goes with it' right after choosing the rewrite, by contradictory 'behind you/ahead' hints, and by a History screen that shows only the pre-rewrite numbers, so the trade-off is invisible afterwards. secondsToObjective values are wall-clock except the summit push, which is game seconds of held W (stepped clock). |
| s02 | You start in a shower late in the afternoon (version 1.1, two open issues, a helper on the way). Keep the software alive and climb while the path is slick, through the rain and on into nightfall. Judge whether the weather is clear (what the slick path does to pushing and rolling) and whether the hill, the modules and the issues stay readable at dusk and at night. Note every moment of confusion and anything hard to see. | achieved | 3 | 28.2 | 0 | 0 | 8 | With a helper, the wet-dusk climb is easy to survive: I shipped 1.2 and 1.3 with nothing going down. The friction came from finding things: issue position hints that flip as you circle the boulder, a 3-issue cap on the HUD list, a strict push cone that gives no feedback in first person, and a boulder and hill that go near-black at night (glowing issue modules are the exception and work well). The rain mostly showed up as halved push speed with little on-screen signal, and it ended quietly within a minute. Times in secondsToObjective are wall-clock (no probe); game time was ~248 s (frame 14888 at 60 fps). |
| s03 | You start early in a game (version 0.2), just as someone from the town has come up to help. Work out from the screen what the helper does and how long they stay, keep climbing and patching while they help, and watch what happens when they move on. Then open the History screen (Tab) and judge whether it tells the story of your climb. Note every moment of confusion. | achieved | 4 | 24.7 | 0 | 0 | 6 | The helper arc works: Kofi patches whatever he can reach, the status line says what he is doing, and his exit line ('Nobody else knew how search worked') followed by search breaking is the best moment of the session. The confusion comes at the start: 'on the way up, 2:30 left' with a frozen clock and no visible helper in first person made me think he was stuck for the first 70 s, and the timer drains on one slope but not the other. History is just a row of numbers and does not tell this climb's story (no releases, no Kofi). secondsToObjective is wall-clock (no probe, stepped clock at a few fps); about 226 s of game time elapsed. |
| s04 | Play as a brand-new player who has never heard of the game. Read the HUD and the first message, work out how to push the boulder and how to patch a module, and get as far up the hill as you can (each terrace is a release: 0.1, 0.2, ...). Note every moment of confusion, and answer the questions at the end. | partial | 3 | 28.1 | 0 | 0 | 6 | The core loop (push to a terrace, ship, patch on the flat) is clear from the first message and worked first try; I reached 0.5 in about 21 wall minutes. The weak point is spatial: finding a module on the far side or underneath, and walking back to the boulder after going round it, where the direction hints lag the view and there is no marker. secondsToObjective are wall-clock seconds from pt start on a slow shared machine (stepped clock), not game time. |

## Answers

Each agent's own words, verbatim from its report; a missing answer is listed under problems.

### Q1. In your own words, what did you think the game was about? What did the boulder, the terraces and the rot stand for?

- **s01:** Maintaining software forever. The boulder is the codebase itself (it literally carries the modules), each terrace is a release that adds a module and users, and the rot is bit-rot/dependency decay (certificates expiring, modules wearing out). The summit is 1.0 and the roll-down is a platform end-of-life; the rewrite is the classic 'start over light and lose your users' trade-off.
- **s02:** It's about keeping software alive: the boulder is the codebase (made of modules like core, auth, mailer), every terrace is a release that adds users and a new module (more weight), and rot is dependency drift, flaky tests, expiring certificates and security advisories that build up whether or not you're pushing. Contributors like Noor help for a while and then leave, taking knowledge with them ('Nobody else knew how mailer worked'). The climb is endless upkeep, the Sisyphus myth applied to maintenance.
- **s03:** Maintaining software forever. The boulder is the codebase: each module is a coloured patch on it, and it gets heavier and harder to keep in reach as it grows. The terraces are releases: each one ships a version, brings more users and adds a module, which means more to look after. The rot is entropy in software, like expiring certificates, flaky tests and dependency drift, which keeps arriving however often you patch ('It will not last'). The helper is a passing contributor: while he is around things feel easy, and when he leaves he takes knowledge with him (bus factor).
- **s04:** It is about maintaining software: the boulder is the codebase you keep pushing forward, each terrace is shipping a release (more users, one more module, more weight), and rot is entropy - certificates expiring, drift - that grows whether or not you touch it. The more you ship the more there is to look after, and the 'It will not last' line plus the summit roll-back say the work is never finished.

### Q2. Did the rot and the upkeep feel fair, or arbitrary? What was hardest to tell or to do?

- **s01:** From this short run it felt fair: decay was slow and visible (health bar, issue list with a percentage), and a helper patched the one open issue for me, so I barely had to patch myself; I could not really judge rot pressure. Hardest was navigation: lining up below the boulder facing uphill at the trailhead took several tries, and the direction hints ('behind you' vs 'ahead') contradicted each other. The '-60%/min left' issue rate was also hard to read.
- **s02:** The rot felt fair. Each issue shows a % and a rate per minute, and it's clear that you can only patch safely on a terrace. New issues popped up quickly but never felt arbitrary, and with the helper it was easy. The hardest part was getting to the module: the 'right / far side / under: roll' hints changed as I moved and I couldn't find sessions; once there were 3 or more issues the HUD hid the rest behind '... and N more lines'; and lining up a push was fussy, with W silently doing nothing when I was slightly off-centre. At night I couldn't see healthy modules or the boulder's outline well. I also couldn't really tell when the rain started or stopped, except that pushing got faster.
- **s03:** Mostly fair: issues come with a rate and a location, and at version 0.2-0.4 with a helper I was never overwhelmed. Search breaking right as Kofi left felt deliberate and thematic, not arbitrary. The hardest part was physical: aiming the crosshair at the right module from up close. The boulder settles and the crosshair drifted from auth onto router, so I patched the wrong module once, and modules under the boulder need it rolled back from the uphill side, which means walking awkwardly around it. Also hard: knowing where Kofi was and why his timer was frozen on one slope but running on the next, and reading the cryptic '-60%/min left' location suffix.
- **s04:** The rot felt fair: each issue shows a percentage and a per-minute rate, so I could plan to finish a climb and patch on the terrace, and patching itself is quick. The hardest part was reaching a module: I misread the location hint ('left' as time left), 'under: roll' was cut off, and when I walked round the boulder to roll it back I could not find my way back to it, so billing rotted from 47% to 23% while I wandered. That felt like a fight with the camera, not with the rot.

## Findings

### 1. [major] After choosing the rewrite, the roll-down banner says 'Every module goes with it' (the other option's text)

- **Category:** text; **reported by** 1 agent(s): s01
- **Detail:** Right after 'Rewrite chosen: it starts again light at the trailhead', the next banner reads 'The platform it runs on reached its end of life. The boulder rolls. Every module goes with it.' This is the description of NOT rewriting, and made me doubt the rewrite had registered. HUD still showed Modules 13 / Users 500 during the roll, so nothing on screen confirmed the rewrite until the trailhead. Expected rewrite-specific roll text (e.g. 'It rolls down to be rewritten').
- **Repro:** pt start ... --from saves/summit; pt hold w 39 (reach summit); pt hold r 3.2; pt wait 5; pt shot
- **Screenshots:** [after5.png](s01/shots/after5.png)

### 2. [major] Getting back to the boulder after walking round it is very hard: no marker, and direction hints lag or contradict the view

- **Category:** ux; **reported by** 1 agent(s): s04
- **Detail:** On the 0.5 terrace I walked round the boulder to roll it back (to reach 'billing ... under'). For ~5 wall minutes (~40 s game time) I could not get back into contact with it: I would turn until it was on screen, hold W, and walk past it. The HUD line 'The boulder: 6 m to your right' and the prompt 'The boulder is behind you, down the hill' were shown while the boulder was visibly right-ahead (back12), and the line disappears entirely when close (uphill4). In the V view the camera swings late after a turn, so screenshots do not show the current heading. billing rotted 47% -> 23% while I was lost. Expected a reliable direction indicator (compass arrow or a current hint) or some help approaching the boulder.
- **Repro:** pt start --profile sisyphus-strata --seed 1 --clock stepped; push to 0.5 (hold w ~110 s total, patch when needed); pt hold s 0.5; pt hold d 2; pt hold w 3.5; then try to turn (pt look) and walk back into the boulder
- **Screenshots:** [nav_fail.png](s04/shots/nav_fail.png)

### 3. [minor] Issue direction hint ('right' / 'far side') flips as you circle the boulder and never led me to sessions

- **Category:** text, ux; **reported by** 3 agent(s): s02, s03, s04
- **Also reported as:** "HUD lines clip at the right edge ('under: roll', '1:43 lef')"; "HUD lines run off the right edge at 640x360 ('under: roll', Noor's '2:28 left')"; "Issue-row location hint ('left', 'far side', 'under: roll') reads as time left and is cut off at the right edge"
- **Detail:** sessions (flaky test) was listed as 'right' from below-left. One metre to the right and facing the boulder it became 'far side', and from the uphill side it was also 'far side'. I spent about 2 minutes of real time walking around a boulder only a few metres across without ever getting the crosshair on it, and Noor patched it in the end. I expected the hint, or a glow I can see from where I stand, to lead me to the module.
- **Repro:** From the wet-dusk save: pt hold d 1.5; pt look -150 0; ... walk around the boulder and watch the sessions line's position word
- **Screenshots:** [side-flip.png](s02/shots/side-flip.png) [rel03_note.png](s04/shots/rel03_note.png) [hud-cutoff.png](s02/shots/hud-cutoff.png) [hud-clip.png](s03/shots/hud-clip.png)

### 4. [minor] Helper line 'on the way up, 2:30 left' reads as 'still coming from town'; Kofi unseen in first person, clock frozen

- **Category:** ux; **reported by** 2 agent(s): s02, s03
- **Also reported as:** "Helper countdown frozen at '2:30 left' while 'on the way up'"
- **Detail:** From load until the boulder reached the 0.3 terrace (~70 s of game time) the HUD said 'on the way up, 2:30 left' and never ticked. Looking down the path in first person showed nobody, so I concluded the helper was stuck. Only the V camera on the terrace showed Kofi standing beside Sisyphus, and the line then changed to 'patching search, 2:29 left'. Expected wording that says he is climbing with me and helps on terraces, and some sign of him in first person.
- **Repro:** pt start --profile sisyphus-strata ... --seed 1 --clock stepped --from saves/helper; pt key F1; pt wait 5; pt look 1200 0; pt step 2; pt shot a (no one on the path, 2:30 unchanged); push to the 0.3 terrace; pt key v; pt step 3
- **Screenshots:** [kofi-arrives.png](s03/shots/kofi-arrives.png) [noor-arrives.png](s02/shots/noor-arrives.png)

### 5. [minor] At 3+ open issues the HUD collapses the rest into '... and N more lines', hiding which modules are failing

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** With 4 issues the HUD showed only 'auth certificate 29%' and then '... and 3 more lines'. I couldn't tell which modules the other issues were on or where they were, even though there is empty screen space below the list. This happened again at 1.3 ('core ... far side / ... and 2 more lines'). Tab history doesn't list current issues.
- **Repro:** From wet-dusk: push to the 1.2 terrace (~60 s of W); pt shot
- **Screenshots:** [more-lines.png](s02/shots/more-lines.png)

### 6. [minor] At night the boulder and the hill are near-black; first person against the boulder is a featureless wall

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** From the 1.2 terrace onwards (night), the boulder is a dark silhouette against a dark hill in the behind view and is mostly hidden behind Sisyphus's dark body. In first person up close it fills the screen with plain black, with no facets or module borders. Healthy modules can be found only through the crosshair prompt. Modules with open issues do stay readable (see the positive finding).
- **Repro:** Play from wet-dusk past the 1.2 release (~80 s game time); pt key v; pt shot
- **Screenshots:** [black-wall.png](s02/shots/black-wall.png)

### 7. [minor] Contributor line says '2:30 left' while he is still walking up from town

- **Category:** text; **reported by** 1 agent(s): s04
- **Detail:** After 0.3 the HUD showed 'Gus is helping: on the way up, 2:30 left' and the number did not change for ~45 s of game time; later Gus was seen walking up the path and the timer only started when he began 'patching search'. The countdown looks frozen/wrong; expected 'Gus is on his way' without a timer, or an ETA.
- **Repro:** pt start ... --seed 1 --clock stepped; push to 0.3; pt hold w 15; compare the Gus line before and after
- **Screenshots:** [gus_frozen.png](s04/shots/gus_frozen.png)

### 8. [minor] Direction hints contradict each other after the roll ('behind you' vs 'ahead')

- **Category:** text; **reported by** 1 agent(s): s01
- **Detail:** On one screen: 'Next 1.1: 33 m uphill (behind you)', 'The boulder: 140 m ahead', and centre prompt 'The boulder is behind you, down the hill: go back down to it'. The centre prompt stays even when the boulder is visibly ahead and HUD says '8 m ahead'; it seems to ignore facing.
- **Repro:** after the rewrite, follow it down with pt hold Shift_L+w 30; pt shot
- **Screenshots:** [back.png](s01/shots/back.png)

### 9. [minor] Helper (Kofi) countdown frozen at 0:49 for ~70 s of game time during the roll down

- **Category:** bug; **reported by** 1 agent(s): s01
- **Detail:** 'Kofi is helping: on the way up, 0:49 left' stayed at 0:49 from the summit through the whole walk down to the trailhead, then resumed counting. Either it should count or say he's paused.
- **Repro:** with a helper present, choose the rewrite and follow the boulder down; watch the Kofi line
- **Screenshots:** [down2.png](s01/shots/down2.png)

### 10. [minor] Helper's time runs out on the 0.3->0.4 slope but not on the 0.2->0.3 slope

- **Category:** balance; **reported by** 1 agent(s): s03
- **Detail:** On the first slope the helper timer stayed at 2:30. On the second slope it counted down while I pushed (1:58 -> 1:43 in 15 s, 'waiting for it to settle'), so his help drained while he could not patch. One rule or the other would be fine, but having both is confusing.
- **Repro:** After shipping 0.3 with Kofi present: walk below the boulder, pt hold w 15; pt shot x (timer has dropped ~15 s)
- **Screenshots:** [kofi-timer-slope.png](s03/shots/kofi-timer-slope.png)

### 11. [minor] History (Tab) shows a single row of numbers that does not tell the story of the climb

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** Mid-climb the History screen reads 'now 0.x 4 10 230 5 0 0 0 1' and 'No climb has finished yet.' It has nothing on the 0.3/0.4 releases, Kofi's name, what he fixed, or that his leaving broke search. 'patched 5' seems to count only my patches (Kofi did ~7). 'helpers 1' is the only trace of him. Because I could not finish a whole climb in the time budget, I did not see a completed row or a cairn.
- **Repro:** Continue to 0.4 with the helper, wait until he leaves, pt key Tab; pt step 3; pt shot h
- **Screenshots:** [history-thin.png](s03/shots/history-thin.png)

### 12. [minor] History screen hides the cost of the rewrite (only pre-rewrite users/modules shown)

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** History marks the climb '1.0*' with a footnote 'rewritten at the summit (modules and users at the end, before the rewrite)'. The rewrite is identifiable, but the drop it caused (500->200 users, 13->6 modules) appears nowhere, so the decision does not read as a trade-off in hindsight; the users chart just looks like decline. Expected a column or line showing users/modules after the rewrite.
- **Repro:** after the rewrite, ship 1.1, then pt key Tab
- **Screenshots:** [history.png](s01/shots/history.png)

### 13. [minor] Shower state is not announced at load; 'Rain: the path is slick' appears only after ~16 s and disappears silently ~45 s later

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** At load the HUD said only 'spring', the sky was bright blue and rain streaks were faint, so the shower the goal describes wasn't visible. The 'Rain: the path is slick' line showed up only after I patched core (~frame 1150), then vanished with no message around frame 2700. The only way I could tell the path was slick was pushing speed: about 0.5 m/s in the rain (36->22 m in ~28 s of W) against about 1 m/s dry later (25->17 m in 8 s). The rain didn't seem to make the boulder slide back any more than usual. I expected a clear 'it starts raining / the rain stops' message and a visible wet look on the path.
- **Repro:** pt start --profile sisyphus-strata ... --seed 2 --clock stepped --from saves/wet-dusk; pt shot start (no rain line); pt hold w 7; pt hold e 5; pt shot (rain line appears); pt hold w 12 (line gone)
- **Screenshots:** [rain-late.png](s02/shots/rain-late.png)

### 14. [minor] Summit choice gives no countdown, and R doubles as 'retire module'

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** The choice text is clear ('It will roll back down with every module aboard. Or hold R to rewrite it: start again light, and lose most users'), but there is no indication how long the pause lasts or how long to hold R. R is the 'retire module' key everywhere else and the crosshair was on the boulder, so I worried I'd retire a module. The Rewriting bar that appears when held is good.
- **Repro:** reach summit; pt shot
- **Screenshots:** [summit_choice.png](s01/shots/summit_choice.png)

### 15. [minor] W into the boulder silently does nothing when slightly off-centre; 'Not lined up' hint loses to the module prompt

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** On the 1.2 terrace I held W three times for 6-10 s and the distance stayed at '33 m'. The boulder was about 50 px left of the crosshair. In first person the prompt area showed only 'mailer: healthy... Nothing to patch', so I got no sign that pushing wasn't working. In the behind view 'Not lined up to push' did show, even though the boulder looked centred behind the character. The boulder also drifts sideways as you push, so this happened again and again (16 s of W gained 2 m). I expected a more tolerant push cone, or the not-lined-up hint to always show while W is held against the boulder.
- **Repro:** After shipping 1.2: get below the boulder, aim ~50 px off its centre, pt hold w 10; HUD 'Next 1.3' doesn't change
- **Screenshots:** [align.png](s02/shots/align.png)

### 16. [polish] At dusk the boulder's module facets are hard to tell apart

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** Late afternoon/dusk in the rain, seen from the uphill side, the facets are almost all one dark purple/navy. I couldn't see where one module ends and the next begins, and the 'sessions' issue on the far side gave no glow I could see.
- **Repro:** From wet-dusk: walk to the uphill side of the boulder (~20 s game time) and face it
- **Screenshots:** [dusk-boulder.png](s02/shots/dusk-boulder.png)

### 17. [polish] Good: helper status messages and the departure toast are clear and land the theme

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** 'nothing they can reach', 'waiting for it to settle', 'patching storage, 2:25 left' told me exactly what Kofi was doing. When he left, the toast 'Kofi moved on. Nobody else knew how search worked.' appeared and search immediately opened a drift issue, which is a strong bus-factor moment. He also took the billing issue as soon as I rolled it into reach.
- **Repro:** Stay on the 0.4 terrace until the helper timer hits 0 (pt wait 65)
- **Screenshots:** [kofi-leaves.png](s03/shots/kofi-leaves.png)

### 18. [polish] Good: onboarding, pushing, releasing and patching are immediately clear

- **Category:** ux; **reported by** 1 agent(s): s04
- **Detail:** The first message ('The boulder is your software. Push it up to the next terrace to ship a release (W, into it)... Hold E on one to patch it.') was enough to play. 'Shipped 0.1. New module: search.' toasts, the 'On the slope it rolls back if you stop: patch on a terrace' hint, and the patch progress bar all worked first try. Stamina appears only after 0.1; it drained slowly and was never a factor in 5 releases.
- **Repro:** pt start ... --seed 1 --clock stepped; pt shot a; pt hold w 30
- **Screenshots:** [patch_ok.png](s04/shots/patch_ok.png)

### 19. [polish] Helper HUD line clipped at right edge; '-60%/min left' hard to parse

- **Category:** text; **reported by** 1 agent(s): s01
- **Detail:** 'Kofi is helping: waiting for it to settle, 1:31 left' runs off screen at 640 wide. Issue row 'mailer certificate 62% -60%/min left' is unclear (rate or time?). 'waiting for it to settle' shown for the whole push, so the helper seems idle while you climb.
- **Repro:** pt start ... --from saves/summit; pt hold w 5; pt shot
- **Screenshots:** [kofi_clip.png](s01/shots/kofi_clip.png)

### 20. [polish] History table shows version '0.x' for the current climb instead of 0.5

- **Category:** text; **reported by** 1 agent(s): s04
- **Detail:** Tab history row 'now 0.x 3 11 310 2 0 0 0 1' while the HUD says version 0.5.
- **Repro:** push to 0.5; pt key Tab; pt shot h
- **Screenshots:** [history_note.png](s04/shots/history_note.png)

### 21. [polish] Module prompt '(hold R to retire it, F to refactor it into parser)' wraps onto two lines

- **Category:** text; **reported by** 1 agent(s): s04
- **Detail:** With a longer module name the retire/refactor hint wraps ('...refactor it into' / 'parser)') under the crosshair; the shorter 'core'/'auth' variants fit on one line.
- **Repro:** push toward 0.3 with the crosshair on the storage module
- **Screenshots:** [push7.png](s04/shots/push7.png)

### 22. [polish] Open-issue suffix '-60%/min left' is cryptic

- **Category:** text; **reported by** 1 agent(s): s03
- **Detail:** Nothing explains that 'left/right/far side/on top/under' means where on the boulder the issue is. On first read '-60%/min left' looked like 'time left'. The rate also did not match what I saw: auth fell 74->68% in 5 s, about 72%/min, perhaps because it depended on a rotting module.
- **Repro:** pt start ...; pt wait 5; pt shot x
- **Screenshots:** [issue-text.png](s03/shots/issue-text.png)

### 23. [polish] Positive: issue modules glow clearly at night and rolling back from uphill shows them well

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** At night a module with an open issue (core, flaky test) glows bright purple with sparks and a floating label, which is very readable. From the uphill side of the 1.3 terrace, the 'Rolling it back to turn it' mechanic plus the glow made core easy to find and patch. Helper messages ('Noor moved on. Nobody else knew how mailer worked.', 'No open issues. It will not last.') are good touches.
- **Repro:** Ship 1.3, walk to the uphill side of the boulder, face it, pt hold w 1.5
- **Screenshots:** [night-glow.png](s02/shots/night-glow.png)

### 24. [polish] Rewrite confirmation shown twice in near-identical words

- **Category:** text; **reported by** 1 agent(s): s01
- **Detail:** 'Rewrite chosen: it starts again light at the trailhead, and most users will not follow' and 'A rewrite. It will start again light at the trailhead.' appear together.
- **Repro:** at summit pt hold r 3.2; pt shot
- **Screenshots:** [r_done.png](s01/shots/r_done.png)

## Objective timings

Game seconds from the start of the world to each objective (event tick / the game's ticks per second, from events.jsonl). Game time, not wall time: with many sessions on one machine a world can run slower than real time, and wall-clock timings would mostly measure the machine.

No objective was completed (or the game has no probe).

