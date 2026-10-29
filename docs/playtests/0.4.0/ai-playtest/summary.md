# Playtest summary: r-sisyphus-5

4 sessions, build sisyphus-strata 0.0 (in-game version shown); session.json version null (8ef2a7f), sisyphus-strata 0.2 (in-game save version); build version null (8ef2a7f), sisyphus-strata 0.6 (in-game version label; build version null) (8ef2a7f), sisyphus-strata 1.1 (in-game software version; game build version not shown) (8ef2a7f), generated 2026-10-29 09:33.

**Findings:** 42 reports in 39 groups -- 0 blocker, 13 major, 22 minor, 7 polish.

## Outcomes

| Session | Goal | Outcome | Fun | Minutes | Objectives | Deaths | Findings | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| s01 | You start near the top of the hill (version 0.6), with the boulder resting on the terrace below the summit. Push it up to the summit and ship 1.0. While the summit pause lasts, the game offers a choice: let it roll back down with every module aboard, or hold R to rewrite it. Work out the choice from the screen, choose the rewrite, follow it down, and play on from the trailhead. Then open the History screen (Tab) and judge whether the rewrite was clear and felt like a real decision. Note every moment of confusion. | blocked | 2 | 22.9 | 0 | 0 | 8 | I never reached the summit, so I could not judge the rewrite choice. One silent slip of the boulder on the last slope set off a collapse I could not recover from: Weight stuck at max no matter what I patched or retired, and the screen never said what drives it. The theme comes through (the 'Kofi moved on. Nobody else knew how search worked.' line is great, and so is 'The boulder is still. For a moment, so are you.'), but feedback on long holds, on losing contact with the boulder and on Weight is missing. The session ran with a free clock rather than stepped, so there are no wall-clock objective times. |
| s02 | You start in a shower late in the afternoon (version 1.1, two open issues, a helper on the way). Keep the software alive and climb while the path is slick, through the rain and on into nightfall. Judge whether the weather is clear (what the slick path does to pushing and rolling) and whether the hill, the modules and the issues stay readable at dusk and at night. Note every moment of confusion and anything hard to see. | partial | 2 | 23.1 | 0 | 0 | 13 | The theme lands. The helper Noor clearing every issue and then leaving ('Nobody else knew how core worked'), the sit view of the town, and 'the rot is the weather' are strong. But from the wet-dusk save the run spirals: the worst issue starts on top of the boulder out of reach, the slick path makes the boulder slip with no warning, and a full weight bar that retiring does not lighten left me no way back (570 users -> 1, no 1.2 release). At dusk and at night the modules are invisible except for glows, which the help bar covers. Times are wall-clock (no probe) and the session ran at a free clock under heavy machine load (~20 fps). |
| s03 | You start early in a game (version 0.2), just as someone from the town has come up to help. Work out from the screen what the helper does and how long they stay, keep climbing and patching while they help, and watch what happens when they move on. Then open the History screen (Tab) and judge whether it tells the story of your climb. Note every moment of confusion. | partial | 2 | 24.7 | 0 | 0 | 12 | The helper moment works at first glance: Kofi visibly walks up, the HUD names what he is patching and counts down, and 'No open issues. It will not last.' is a great line - but his departure is silent and his HUD line already vanishes whenever issues pile up. After he left, a lone player could not keep up: the boulder slipped off pushes twice, F/refactor did nothing, and rot cascaded to 0 users and 'too heavy to push' with no fail state or way out signposted; History is a single stats row that does not tell that story. secondsToObjective is wall-clock (no probe); the clock was 'free', not stepped. |
| s04 | Play as a brand-new player who has never heard of the game. Read the HUD and the first message, work out how to push the boulder and how to patch a module, and get as far up the hill as you can (each terrace is a release: 0.1, 0.2, ...). Note every moment of confusion, and answer the questions at the end. | partial | 2 | 26.2 | 0 | 0 | 9 | The premise and writing land right away: boulder = software, rot = upkeep, and patching feels good. But I never shipped 0.1 in 26 minutes, because the push is too easy to lose (the boulder slides off sideways and rolls back with no warning) while rot at version 0.0 runs at 30-45%/min and sends weight and users into a death spiral. Finding the module the HUD names ('facing you', 'on top') was the hardest part. Make the first climb a forgiving tutorial (slow rot, sticky push, highlight the worst module) before the treadmill starts. secondsToObjective is wall-clock (no probe). |

## Answers

Each agent's own words, verbatim from its report; a missing answer is listed under problems.

### Q1. In your own words, what did you think the game was about? What did the boulder, the terraces and the rot stand for?

- **s01:** Maintaining software is an endless, Sisyphean push. The boulder is the codebase, carrying every module. The terraces are releases (0.1 ... 1.0), resting points where you can stop and do upkeep. Rot is dependency drift, security advisories and bit-rot that build up whenever nobody tends a module. Users leave when modules go down. Contributors like Kofi help for a while and then leave, taking their knowledge with them ('Nobody else knew how search worked').
- **s02:** Maintaining software as an endless Sisyphean chore. The boulder is your codebase: every module is a patch of its surface, and it gets heavier as you add features or let things go bad. The terraces are releases, resting places where your progress is safe. The rot is the bit rot / dependency drift / expiring certificates that happen to working software anyway; as the sit-screen text says, it is 'the weather', not a failure. Users leave when things are down, and helpers (contributors) come and go.
- **s03:** Maintaining software forever. The boulder is the codebase (its faces are modules that depend on one another), each terrace is the next release (0.3 was 36 m up), and the rot is bit-rot: expiring certificates, deprecated APIs, dependency drift, flaky tests. Users leave when something is down. The helper is an outside contributor who shows up, fixes things for a couple of minutes and leaves. The Camus quote when you sit makes the point: the work never ends.
- **s04:** It's about maintaining software forever. The boulder is the codebase. Each terrace is a release that adds a module, which makes the next climb harder. Rot is software entropy: deprecated APIs, flaky tests, dependency drift. It makes the boulder heavier until you can't make progress, and users leave when something is down. Module dependencies ('uses core', 'uses auth') stand for how one broken piece drags down the ones that use it. The intro line 'It will not last' and the sit message make it feel like a gentle joke about never being done.

### Q2. Did the rot and the upkeep feel fair, or arbitrary? What was hardest to tell or to do?

- **s01:** It felt unfair in this session. Rot is fast (-36 to -73%/min, and much faster once a dependency is down), and my one mistake, losing the boulder on the slope, cascaded into everything being down within minutes. Hardest of all: knowing whether a held E/R was working (no visible progress after a short hold; it needed about 15 s), finding a module the HUD calls 'far side' or 'facing you' on a dark boulder with few labels, telling uphill from downhill around the boulder on a terrace, and understanding Weight, which stayed at max whatever I did, so 'too heavy to push' was a dead end I could not read my way out of.
- **s02:** The rot rates are shown and readable (e.g. -37%/min), and patching a module you can reach is quick and satisfying, so the core loop feels fair. What felt arbitrary was where things are: issues 'on top' or 'under' the boulder that you cannot reach without rolling it, rolling that overshoots, and a slick path that makes the boulder slip with no warning. Hardest of all were finding which part of the dark boulder a HUD line meant at dusk and night, and understanding why the boulder stayed 'too heavy' after I retired modules. Once it was too heavy I could not tell what would get me out.
- **s03:** The cause and effect was fair and readable: a module going down made its users rot faster, and the HUD said which one and where it was. The pace did not feel fair alone, though. Once the helper left, one person could not keep 8 modules patched and push at the same time, and it cascaded to zero users. The hardest things were reaching modules on the far side or underneath (the crosshair targeting and the relative 'left/right/far side' words), keeping the boulder in front of me while pushing, and knowing why F/refactor did nothing. I could not tell whether there was any way back from 'too heavy to push uphill' other than retiring modules.
- **s04:** Arbitrary, at least at version 0.0. Rot started before I'd learned to push, ran at 30-45% a minute, and a module I'd just patched got a new issue within seconds, so I never reached the first terrace. The idea was clear and the reasons for each issue were readable, but the hardest thing was finding the module the HUD named. 'On top', 'far side', 'facing you' are relative to where you stand, and in first person and at night the modules are unlabelled and look alike. Keeping the push on line, without the boulder slipping sideways and rolling back, was the other big difficulty.

## Findings

### 1. [major] Boulder slips off the push and rolls back with no feedback; twice lost all progress on the first slope

- **Category:** ux; **reported by** 2 agent(s): s03, s04
- **Also reported as:** "Pushing up the first slope: boulder repeatedly slides off the push and rolls back to the trailhead"
- **Detail:** Holding W from directly below (no direction hint after 'Next 0.3', i.e. facing uphill): progress 36 -> 19 m, then the boulder drifted sideways, I kept walking up the grass and it rolled back to the bottom. Second attempt 28 -> 23 m, same result. No 'it slipped' message, no stamina bar visible during the push; the only cue was 'Next' jumping back and a new 'The boulder: 19 m behind you' line. Expected the push to keep the boulder centred or warn when contact is lost.
- **Repro:** get below the boulder facing uphill; pt hold w 8 (twice); pt shot
- **Screenshots:** [slid-off.png](s03/shots/slid-off.png) [lost_boulder.png](s04/shots/lost_boulder.png)

### 2. [major] Death spiral after one slip: weight bar full ('too heavy to push uphill') and retiring modules does not lighten it

- **Category:** balance; **reported by** 2 agent(s): s02, s04
- **Also reported as:** "Rot outpaces a new player at version 0.0: weight death-spiral to 'too heavy to push uphill' and 0 users before the first release"
- **Detail:** After the boulder slipped and a few modules went down, Weight went to [##########] 'too heavy to push uphill'. Retiring uploads (14 -> 13 modules), and later a DOWN module (13 -> 12), left the bar full. With DOWN modules under the boulder or on top and the boulder unpushable, I found no way back. Users dropped from 570 to 1 in about 15 minutes. The HUD never says what makes the boulder heavy or what would lighten it.
- **Repro:** continue from slipped state; aim at a module; pt hold r 7; observe Weight bar unchanged
- **Screenshots:** [weight_full.png](s02/shots/weight_full.png) [n6.png](s04/shots/n6.png)

### 3. [major] Holding F (refactor) does nothing, with no progress bar or reason

- **Category:** bug; **reported by** 2 agent(s): s03, s04
- **Also reported as:** "Holding E sometimes does nothing on a module whose prompt says 'Hold E to patch' (no bar, no reason)"
- **Detail:** On 'router: DOWN (0%), uses auth' with the prompt 'F to refactor it into auth', held F for 5 s, then 8 s; then again for 6 s once router was healthy (99%). Nothing changed, no bar, no message. E (patch) and R (retire) on the same boulder worked fine. Either refactor is blocked by a condition (auth was flaky at the time?) that the prompt does not explain, or it is broken.
- **Repro:** aim at router (prompt offers F to refactor into auth); pt hold f 8; pt shot
- **Screenshots:** [refactor-nothing.png](s03/shots/refactor-nothing.png) [k9.png](s04/shots/k9.png)

### 4. [major] After the helper leaves, solo upkeep collapses into an unrecoverable death spiral with no fail state

- **Category:** balance; **reported by** 1 agent(s): s03
- **Detail:** With Kofi helping, issues cleared ('No open issues. It will not last.'). About 2.5 min after he left, outages cascaded (auth -> router -> storage -> search -> core). Within ~12 real minutes users went 100 -> 0, health 92% -> 1-13%, weight hit 'too heavy to push uphill', 6-8 open issues, several marked 'under: roll it' that cannot be rolled into reach because it is too heavy. The game just keeps going at 0 users with no ending, no prompt and no hint (e.g. 'retire something') of how to dig out. Retiring (R) did work and is the only lever I found, but nothing pointed me to it.
- **Repro:** pt start --profile sisyphus-strata ... --seed 1 --from saves/helper; play normally for ~20 min (push, patch what is reachable)
- **Screenshots:** [spiral.png](s03/shots/spiral.png)

### 5. [major] At dusk/night the modules on the boulder are invisible; only the crosshair prompt identifies them

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** Up close at dusk the boulder is near-black polygons. The HUD said 'storage ... facing you', but nothing on the surface showed where storage was, so I had to sweep the crosshair blind. Only rotting/DOWN modules glow (purple/orange/red), and some show floating name labels. At full night the boulder is a black silhouette with red glows. The HUD text itself stays readable at night.
- **Repro:** load wet-dusk; walk to the boulder; pt shot; pt look small steps
- **Screenshots:** [night_modules.png](s02/shots/night_modules.png)

### 6. [major] Boulder silently slips away mid-push on the final slope; the player keeps walking uphill without it

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** From the 0.6 save I held W for 5 s, then 15 s (HUD: 36 -> 30 -> 14 m to the summit). After another 12 s of W the HUD said 'Next 1.0: 28 m uphill' and 'The boulder: 38 m behind you'. The boulder had rolled all the way back to the 0.6 terrace while I walked on. Nothing on screen marked the moment I lost contact: no message, no sound cue (the session is silent), no camera cue. I expected a clear 'it is slipping' warning, or the push to stop when I drift off the boulder.
- **Repro:** pt start --profile sisyphus-strata ... --seed 1 --from saves/summit; pt hold w 5; pt hold w 15; pt hold w 12; pt shot
- **Screenshots:** [lost_boulder.png](s01/shots/lost_boulder.png)

### 7. [major] Module position words (on top / far side / left / right / facing you) don't help find the module to aim at

- **Category:** ux; **reported by** 1 agent(s): s04
- **Detail:** The positions are relative to the player and change as you walk. 'on top' storage turned out to be patchable from the uphill side at chest height. Core was 'facing you' at 18% and falling, but every aim I tried found only ui. In first person the modules have no name labels and look alike, and at night they're nearly invisible. Finding the broken module took longer than walking up the hill. I expected the crosshair to snap or highlight toward the worst issue, or labels in first person.
- **Repro:** pt start ... --seed 1; let core rot; stand at boulder in first person where HUD says 'core ... facing you'; pt look in various directions; prompt keeps saying 'ui: aging ...'
- **Screenshots:** [t5.png](s04/shots/t5.png)

### 8. [major] One lost push spirals into total collapse within ~3 minutes; Weight then stays maxed and the climb is unrecoverable

- **Category:** balance; **reported by** 1 agent(s): s01
- **Detail:** After the slip, walking back down and lining up again took about 3 minutes. In that time router, storage and cache went DOWN, users fell 400 -> 124, and Weight went to max ('too heavy to push uphill'; on pushing: 'Too heavy. It slides back.'). After that I patched ui, storage and cache to 97-99% and retired ui, search, router and uploads (12 -> 8 modules), and Weight never moved off max. Users went to 0 and health to 0%. The screen never explains what Weight is made of or what lowers it, so I had no way to get the boulder to 1.0 and never saw the summit or the rewrite choice. (The session ran with clock=free, not stepped as the brief says, which made this worse for a screenshot-driven player.)
- **Repro:** after the slip above: walk back to the boulder on the 0.6 terrace (about 3 min of turning and walking); pt hold e 15 on modules; pt hold r 15 on DOWN modules; pt hold w 6 from below
- **Screenshots:** [push_heavy2.png](s01/shots/push_heavy2.png)

### 9. [major] Opening state of wet-dusk save loses core almost immediately: worst issue is 'on top' and out of reach

- **Category:** balance; **reported by** 1 agent(s): s02
- **Detail:** On load the worst issue is 'core drift 21% -37%/min on top'. From below the boulder you cannot aim at the top (looking straight up only hits billing), and pushing uphill rolls it to the far side. Core went DOWN within ~30 s of loading. The session also ran with clock 'free' (session.json), not stepped as the brief says, so it rots in real time while a player is still reading the HUD. Nothing on screen says how to reach an 'on top' module.
- **Repro:** pt start --profile sisyphus-strata ... --seed 2 --from saves/wet-dusk; pt shot start; pt look 0 -350; pt shot up2
- **Screenshots:** [core_on_top.png](s02/shots/core_on_top.png)

### 10. [major] Patch/retire holds of 3-8 s show nothing at all, and progress is lost on release; it took ~12-15 s

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** With the prompt 'Hold E to patch' on screen I held E for 5, 8, 6, 4 and 3 s (and R and F for 5 s): no visible change and no bar in any screenshot after release, so I concluded E was broken (see e_noop). Holding for 12-15 s did patch (ui 5% -> 99%, storage 0% -> 99%, cache 0% -> 97%), and 15 s of R did retire. The brief and the game imply 1-5 s. At the ~7 fps the software renderer gave, the real time needed is much longer, and because a short hold leaves no partial progress, the player cannot tell 'nearly done' from 'not working'.
- **Repro:** aim at a damaged module within reach; pt hold e 5; pt shot (no change); pt hold e 15; pt shot (patched)
- **Screenshots:** [e_noop.png](s01/shots/e_noop.png)

### 11. [major] Slick path: the boulder slips away with no warning or visible cue

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** While I held W into the boulder at 24 m, it slid past me and ended up 10 m downhill ('The boulder: 10 m behind you', Next 1.2 back to 27 m). Earlier, letting go mid-slope at 30 m did NOT roll it back, and once 'It is rolling back down!' showed with no movement. The rain is the only stated cause ('Rain: the path is slick'), but there is no slip animation, sound cue or meter, so the slipperiness feels random rather than readable. Pushing was also very slow: about 2-3 m per 6-8 s of holding W.
- **Repro:** from below the boulder facing uphill: pt hold w 6; pt hold w 8; pt hold w 4; pt shot
- **Screenshots:** [slipped.png](s02/shots/slipped.png)

### 12. [minor] 'From up here, walking into it rolls it back' appears while facing uphill beside the boulder

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** Several times, with the HUD saying '28 m uphill' straight ahead (I was facing up the hill), the prompt said I was above the boulder and that W would roll it back. Uphill and downhill around the boulder on a terrace were hard to read from first person, and worse at night and in rain.
- **Repro:** stand beside the boulder on the 0.6 terrace facing up the hill
- **Screenshots:** [v2.png](s01/shots/v2.png)

### 13. [minor] 'facing you' modules can sit on the silhouette edge where the crosshair can barely hit them

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** HUD said 'ui DOWN facing you', but its red glow showed only as sparkles along the left silhouette edge. Several look moves kept the crosshair on 'search'. Mouse look also moved unevenly per pixel at this frame rate (80 px barely moved, 200 px overshot).
- **Repro:** pt look -80 -10; pt look -200 -30; pt look 110 0; pt shot
- **Screenshots:** [cant_aim_ui.png](s02/shots/cant_aim_ui.png)

### 14. [minor] At the trailhead the boulder can't be rolled downhill to turn it, so the guidance to roll it on level ground doesn't work there

- **Category:** ux; **reported by** 1 agent(s): s04
- **Detail:** The prompt 'From up here, walking into it (W) rolls it back (on level ground)' appears at the trailhead, but pushing from the uphill side didn't move it ('Next 0.1' stayed 27 m and storage stayed 'on top'). As a new player I read this as the way to bring the top module round, and it didn't work.
- **Repro:** with the boulder at the trailhead, stand on its uphill side facing down; pt hold w 4; HUD unchanged
- **Screenshots:** [l2.png](s04/shots/l2.png)

### 15. [minor] Behind-view camera (V) clips inside the boulder when it is behind Sisyphus

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** With the boulder right behind the player, the V camera sits inside it: the whole screen is dark-blue polygons and you cannot tell where you are or which way you face.
- **Repro:** stand with the boulder at your back; pt key v; pt shot
- **Screenshots:** [cam_in_boulder.png](s02/shots/cam_in_boulder.png)

### 16. [minor] Controls help bar covers the glow of rotting modules at the boulder's base

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** The purple glow of the rotting storage module was right behind the F1 controls bar (on by default after load). I found it only after hiding help with F1 and looking down.
- **Repro:** stand 1-2 m from boulder; pt shot (help visible); pt key F1; pt look 0 120; pt shot
- **Screenshots:** [help_hides_glow.png](s02/shots/help_hides_glow.png)

### 17. [minor] Crosshair prompt names a module but holding E sometimes does nothing, with no feedback

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** With the prompt reading 'core: dependency drift ... Hold E to patch', I held E for 8 s, 10 s and 6 s. No progress bar or message appeared, and core kept falling (63% -> 58%). After a tiny re-aim the same hold patched core to 99%. The prompt probably lags the real ray hit at the low frame rate, but the player gets no sign of why E is ignored.
- **Repro:** aim near the core glow at the bottom edge of boulder; pt hold e 8; pt shot
- **Screenshots:** [patch_no_effect.png](s02/shots/patch_no_effect.png)

### 18. [minor] F (refactor) held 7 s does nothing and gives no message

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** Aimed at uploads ('F to refactor it into router') and held F for 7 s. No progress bar, no message, module count unchanged. R on the same aim worked immediately ('Retired. One less thing to keep...').
- **Repro:** aim at uploads; pt hold f 7; pt shot
- **Screenshots:** [weight_full.png](s02/shots/weight_full.png)

### 19. [minor] Helper status line disappears from the HUD when several issues are open, so the helper seems to have left

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** Start: 'Kofi is helping: on the way up, 2:30 left'. With 3 open issues the Kofi line vanished entirely (shots d1/d2) and I thought he had gone; a minute later it came back as 'Kofi is helping: patching ui, 2:16 left'. Also 'on the way up, 2:30 left' did not count down while he walked up, and his leaving had no message at all - the line just was not there any more.
- **Repro:** start from saves/helper; look around the boulder for ~30 s while issues accumulate; pt shot
- **Screenshots:** [kofi-back.png](s03/shots/kofi-back.png)

### 20. [minor] History (Tab) is a stats row, not the story of the climb; helper is not named

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** History shows one row: 'now 0.x 19 min, 7 modules, 0 users, patched 11, retired 1, refactored 0, down 13, helpers 1' plus 'No climb has finished yet.' It does not say Kofi came, how long he stayed or what he patched (patched 11 seems to merge his and mine), nor when outages hit or users left. Version shows 'now 0.x' rather than 0.2 -> 0.3. Text is drawn straight over the 3D scene with no backing panel.
- **Repro:** after a few minutes of play, pt key Tab; pt shot
- **Screenshots:** [hist-numbers.png](s03/shots/hist-numbers.png)

### 21. [minor] Issue placement hint cut off at the right edge of the HUD

- **Category:** text; **reported by** 1 agent(s): s02
- **Detail:** Lines like 'i18n drift 91% -36%/min under: roll' run off the 640 px window, so the 'roll it' hint is truncated. Only 2 issues are listed, then '... and N more lines', and I found no way to see the rest.
- **Repro:** get an issue that is under the boulder; pt shot
- **Screenshots:** [cutoff_under.png](s02/shots/cutoff_under.png)

### 22. [minor] Issue row direction word reads as part of the rate ('-60%/min left')

- **Category:** text; **reported by** 1 agent(s): s03
- **Detail:** 'auth  certificate 66%  -60%/min left' reads as 'time left'. 'left' is actually where the module is on the boulder (later 'far side', 'facing you', 'under: roll'). The directions are also relative in a way I misread repeatedly: strafing the way I thought 'left' meant moved it to 'far side'.
- **Repro:** pt start ...; pt shot start
- **Screenshots:** [hint-text.png](s03/shots/hint-text.png)

### 23. [minor] Module labels on the boulder show for only a few modules; 'core: facing you' could not be found

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** The HUD said core and auth were 'facing you', but only some modules carry a floating label, and at night most of the boulder is dark red dots. I swept the crosshair and only ever found auth and mailer, never core.
- **Repro:** stand next to the boulder with core 'facing you'; sweep pt look in small steps
- **Screenshots:** [zero_users.png](s01/shots/zero_users.png)

### 24. [minor] Modules on the boulder's rim or underside are very hard to put the crosshair on

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** The red dot trail and the floating module label often sit at the silhouette edge or under the boulder; aiming at them gives a different module's prompt or 'From up here, walking into it rolls it back'. It took 5-8 look/strafe attempts per module. Mouse-look yaw also felt inconsistent (sometimes 200 px turned a few degrees, sometimes much more).
- **Repro:** with an issue marked 'right' or 'under: roll', try to target it with pt look / pt hold a\|d
- **Screenshots:** [stuck-heavy.png](s03/shots/stuck-heavy.png)

### 25. [minor] Night is near-black on the software renderer

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** About 8 minutes in, night fell and the hill and boulder became almost black; only module glows remained, making orientation very hard.
- **Repro:** play ~8 min real time; pt shot
- **Screenshots:** [night-dark.png](s03/shots/night-dark.png)

### 26. [minor] Nights are nearly pitch black in first person; boulder and modules are invisible up close

- **Category:** ux; **reported by** 1 agent(s): s04
- **Detail:** Up against the boulder at night the screen is almost solid black. I can't tell where the crosshair is or which module it's on, and the prompt falls back to 'Face the boulder and walk into it'. The day/night cycle is only a few minutes long, so this comes up a lot.
- **Repro:** play until night; walk into the boulder in first person; pt shot
- **Screenshots:** [night_dark.png](s04/shots/night_dark.png)

### 27. [minor] No end beat when users hit the floor: users stuck at 1, game just idles

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** With 12 issues open and users at 1, nothing happens: no failure moment, no prompt to roll it back or rewrite. Users never reached 0.
- **Repro:** let all modules go down; pt wait 90; pt shot
- **Screenshots:** [night_end.png](s02/shots/night_end.png)

### 28. [minor] No stamina bar while pushing; can't tell exhausted from too heavy from not touching

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** The HUD shows a Stamina bar at start, but it was gone during pushes. Holding W into the boulder moved it 0-1 m over 8 s with weight full but before the explicit 'too heavy to push uphill' text appeared, so I couldn't tell why.
- **Repro:** with weight near full, pt hold w 8 from below; pt shot
- **Screenshots:** [stuck-heavy.png](s03/shots/stuck-heavy.png)

### 29. [minor] Open-issues list shows only 2 lines plus '... and 5 more lines'; I could not see the other DOWN modules or where they were

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** With 8-10 open issues the HUD lists only the worst two (often both DOWN) and hides the rest. The order also reshuffles. Locations ('under: roll', 'far side', 'left', 'facing you') are relative to where you stand and change as you move. Planning which module to reach next was guesswork.
- **Repro:** let several modules rot; read the HUD
- **Screenshots:** [hud_list.png](s01/shots/hud_list.png)

### 30. [minor] Shower and dusk are very short: rain gone in ~2 min, full day-night cycle in ~10-15 min

- **Category:** balance; **reported by** 1 agent(s): s02
- **Detail:** 'Rain: the path is slick' was gone from the HUD about 2 minutes after loading. The sky went dark dusk -> bright midday -> dusk -> night in about 20 real minutes. The 'wet dusk' set-up barely lasts long enough to judge the weather.
- **Repro:** load wet-dusk; pt wait / play ~20 min; compare shots
- **Screenshots:** [sky_day.png](s02/shots/sky_day.png)

### 31. [minor] Zero users and 0% health has no consequence or message

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** Users reached 0 and health 0% with every module DOWN. No message, no fail state, no hint. The game just carries on, with the boulder stuck and too heavy. I expected either a hint toward the way out (rewrite? retire?) or an explicit end of this climb.
- **Repro:** let all modules go down
- **Screenshots:** [dead.png](s01/shots/dead.png)

### 32. [minor] pt hold w 3.2 timed out ('no answer to event within 60s'), and W seemed to stay held for a minute

- **Category:** perf; **reported by** 1 agent(s): s04
- **Detail:** After the hold timed out, Sisyphus had walked 44 m past the boulder into the town and night had fallen. Health fell 69% -> 52% during it. The game runs about 6-13 fps on the software renderer (clock was 'free', not stepped as the brief said), and small pt look moves (<100 px) often seemed not to turn the view.
- **Repro:** pt hold w 3.2 under load; pt status shows frame ~3254 after 9 min
- **Screenshots:** [stuck_key.png](s04/shots/stuck_key.png)

### 33. [polish] Floating module labels overlap the crosshair prompt text

- **Category:** text; **reported by** 1 agent(s): s02
- **Detail:** The 'router' label on the boulder is drawn over the prompt text ('i18n: nearly new (97%)'), garbling both.
- **Repro:** aim just above a labelled module; pt shot
- **Screenshots:** [label_overlap.png](s02/shots/label_overlap.png)

### 34. [polish] HUD helper line overflows the right edge

- **Category:** text; **reported by** 1 agent(s): s03
- **Detail:** 'Kofi is helping: waiting for it to settle, 1:06 left' is clipped at 640 px.
- **Repro:** push the boulder while the helper is present; pt shot
- **Screenshots:** [hud-overflow.png](s03/shots/hud-overflow.png)

### 35. [polish] Issue table columns collide

- **Category:** text; **reported by** 1 agent(s): s03
- **Detail:** 'search  uses storage86%' - the problem text runs into the percent column. Floating world labels also overlap ('billingsearch').
- **Repro:** let a module become 'uses storage' while storage is down; pt shot
- **Screenshots:** [col-overlap.png](s03/shots/col-overlap.png)

### 36. [polish] Positive: patching, rot flavour text, sit and history all read well

- **Category:** ux; **reported by** 1 agent(s): s04
- **Detail:** The intro text explains the premise in one go. Patching gives instant, satisfying feedback (43% -> 99%). Rot reasons ('deprecated API, a call it makes is going away', 'a dependency fell three majors behind') are funny and true. Q sit ('The boulder is still. For a moment, so are you.') is a lovely beat. Tab history is clear.
- **Repro:** pt key q; pt key Tab
- **Screenshots:** [sit.png](s04/shots/sit.png)

### 37. [polish] Prompt says 'aging, no issue yet (5%)' while the HUD lists the same module as 'rotting 5%'

- **Category:** text; **reported by** 1 agent(s): s04
- **Detail:** Router at 5% was in the HUD's open issues as 'rotting', but the crosshair prompt said 'aging, no issue yet'. Also, right after a patch a module shows as an open issue at 98% while the prompt says 'Nothing to patch'.
- **Repro:** let router rot to ~5% with no named issue; aim at it
- **Screenshots:** [r8.png](s04/shots/r8.png)

### 38. [polish] Sitting (Q) turns the view to face downhill, which reorients the player

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** After sitting and standing again, the summit was 'behind you' and I had to find the boulder again. Thematically nice (you look at the view), but costly when time matters.
- **Repro:** pt key q; pt wait 3; pt key w; pt shot
- **Screenshots:** [stand.png](s01/shots/stand.png)

### 39. [polish] Terrace signposts '0.1'/'0.2' read like cairns

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** Floating orange '0.1' and '0.2' labels near the town looked like the cairns the help text mentions; walking down showed they are terrace signposts. Combined with History saying 'No climb has finished yet', this was briefly confusing.
- **Repro:** walk downhill from the start toward the town; pt shot
- **Screenshots:** [signposts.png](s03/shots/signposts.png)

## Objective timings

Game seconds from the start of the world to each objective (event tick / the game's ticks per second, from events.jsonl). Game time, not wall time: with many sessions on one machine a world can run slower than real time, and wall-clock timings would mostly measure the machine.

No objective was completed (or the game has no probe).

