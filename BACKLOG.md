# Until Last Asteroid — Development Backlog

This file is the source of truth for completed fixes, current patch work, and
deferred ideas. Update it whenever an item changes scope or release.

## Released

### v1.0.1

- Fixed unmatched input events being reported as handled.
- Allowed enemies to spawn on every screen edge.
- Prevented levels from completing before pending enemies and waves were gone.
- Awarded score only for enemies destroyed by player shots.
- Prevented duplicate enemy-shot sounds.
- Ignored player controls outside active gameplay.

### v1.0.2

- Clamped long frame updates.
- Guarded letterbox calculations against zero-size resize events.
- Recentered dynamic status text after changing its contents.
- Removed the unused keyboard shoot action.
- Added temporary protection and blinking after player respawn.

### v1.0.3

- Preserve `Game Over` when the player and the last enemy are destroyed together.
- Stop the current frame immediately after the game window closes.
- Remove the remaining `C4267` conversion warning in meteor texture selection.
- Match shot texture paths to the exact on-disk directory casing.
- Avoid copying the input binding map during event and realtime processing.
- Remove confirmed unused code.

### v1.0.4

- Upgraded SFML from 3.0.2 to 3.1.0.
- Built the x64 Debug and Release libraries with the Visual Studio `v145`
  toolset.
- Confirmed source compatibility and packaged only the four runtime modules used
  by the game.
- Kept the existing Visual Studio project setup without migrating to CMake or
  vcpkg.

### v1.1.0

- Introduce an explicit application state machine.
- Add the skippable Alone Bull Company splash with fade-in, hold, fade-out,
  and its accompanying sound.
- Add the first functional main menu with Start Game, Scores, Options, and
  Quit.
- Animate the title letter by letter with typing sounds before moving it to the
  top of the menu.
- Type menu labels sequentially, reveal their interface frames, and start the
  menu ambience after the interface activation sound.
- Display the current game version in small text at the bottom-right.
- Move music and sounds under `assets/audio/` and update all asset paths.
- Standardize asset file names using `snake_case`.
- Use Orbitron for the title and menu controls.
- Reserve the lower-left for menu controls and use a layered space-and-asteroid
  parallax background.
- Add menu navigation and confirmation sounds.
- Add custom sci-fi cursors for the menu and active gameplay.
- Add an explicit paused game state.
- Pause and resume active gameplay with Escape.
- Pause automatically when the window loses focus or is minimized.
- Disable movement and shooting while paused.
- Add a lower-left pause menu with Resume and Back to Main Menu.
- Blur the captured gameplay frame at the native viewport resolution with a
  cached two-pass shader, then darken it behind the pause interface.
- Pause gameplay music and active sound effects, then restore the gameplay
  cursor and audio on resume.
- Resume without a large frame-time jump.

#### Release verification

- Completed the manual v1.1.0 regression checklist.
- Prepared, extracted, smoke-tested, and approved the v1.1.0 release candidate.

### v1.2.0

- Replaced the Options placeholder with Graphics, Audio, and Controls pages.
- Added supported display resolution discovery and Fullscreen, Windowed, and
  Borderless modes.
- Disabled resolution selection in Borderless mode and explained why in the UI.
- Added Show FPS, Vertical Synchronization, and Frame Rate Limit settings.
- Applied settings immediately, with a timed safety rollback for display mode and
  resolution changes.
- Added Music and Sounds volume sliders backed by a centralized audio manager.
- Added per-asset audio balancing through JSON configuration.
- Added keyboard and mouse rebinding with conflict handling and defaults.
- Stored validated JSON settings under the current Windows user's LocalAppData.
- Added reusable dropdown, slider, segmented toggle, and key-binding controls.
- Added a focused UI bloom pass for selected controls, the title, cursor, and
  gameplay crosshair.
- Deferred Scores until the campaign and scoring rules are finalized.
- Deferred Gameplay options until the game has meaningful gameplay and
  accessibility settings.

#### Release verification

- Completed the manual v1.2.0 regression checklist.
- Built and verified the x64 Debug and Release configurations.
- Prepared and verified the standalone Windows release package.

### v1.3.0

- Replace the player's three lives with 100 health and a lower-left health bar.
- Color the health fill from green through yellow to red and show its percentage.
- Flash the health fill for three seconds when health reaches the critical range.
- Give the player one second of blinking invulnerability after taking damage.
- Give every asteroid and enemy type its own health, contact damage, and score value.
- Make player and enemy projectiles deal configured damage and knock targets back.
- Flash surviving enemies briefly when a hit is registered.
- Separate and knock back the player and enemy when both survive a collision.
- End the run at zero health without respawning the player in the center.
- Move player, weapon, enemy, and level balance into validated JSON data files.
- Replace the player and enemy shot sounds with the new combat audio set.
- Use one ship explosion sound for the player and enemy saucers.
- Use one asteroid explosion sound with a higher pitch for smaller asteroids.
- Add separate hull-impact sounds for asteroids and enemy saucers, played only
  when collision damage is accepted.
- Trim the leading silence from the asteroid explosion and lower the player-shot
  resource volume.
- Replace the temporary gameplay theme with `gameplay_background_1.ogg` on all
  current levels; reserve the other tracks for the future campaign design.
- Scale the company splash to the complete physical window at every resolution.
- Rename the executable to `UntilLastAsteroid.exe` to avoid a stale Windows/Intel
  presentation profile that forced the old `Asteroid.exe` to the monitor's 60 Hz
  refresh rate in Fullscreen and Borderless modes.

#### Release verification

- Completed the full manual gameplay and regression checklist for
  `v1.3.0-rc.1` without finding release-blocking issues.
- Built and verified the x64 Debug and Release configurations.
- Prepared, extracted, file-verified, and smoke-tested the standalone Windows
  release candidate.

### v1.4.0

- Replace the black gameplay field with a unique 4K space background for every
  current level plus procedural star and dust parallax.
- Replace the player, enemy, asteroid, and projectile artwork with a cohesive
  high-resolution top-down sci-fi set.
- Reduce asteroids to two gameplay sizes: large asteroids split into two small
  asteroids, while small asteroids no longer split. The former medium art and
  balance now define the small size; the former tiny size is removed.
- Keep large asteroids, kamikazes, and shooters in the Level 1 initial spawn
  during visual-development testing; rebalance this encounter when the campaign
  and its onboarding progression are designed.
- Make the circular kamikaze saucer spin while pursuing the player, and make the
  shooter gunship alternate between its two JSON-configured weapon emitters.
- Decouple explicit collision radii and visual scales from source PNG bounds.
- After every new gameplay sprite is approved, review its silhouette and replace
  provisional circle colliders where needed. Compare oriented boxes, capsules,
  and compound circles. Per the final art review, perform this collider pass near
  the end of v1.4.0 after the visual-effects and background work.
- Use three JSON-configured rotating circles for the elongated player and the
  wide shooter; retain single-circle collision for the circular kamikaze,
  asteroids, and projectiles.
- Add a reusable batched particle system for engines, muzzle flashes, hits, explosions,
  smoke, sparks, and debris.
- Add controlled gameplay bloom for emissive projectiles, engines, hits, and
  explosions without blurring the HUD.
- Give stone impacts, metal impacts, asteroid destruction, and ship destruction
  distinct particle recipes, with their tuning values stored in validated JSON.
- Play `metal_hit.ogg` for non-lethal projectile damage to player and enemy
  ships, and remove the unused enemy-spawn sound events.
- Play `bullet_hit_asteroid.ogg` for non-lethal projectile damage to asteroids,
  using the asteroid-size pitch variation without replacing hull-impact audio.
- Add bounded camera shake for damage and major explosions.
- Move level backgrounds and visual-effect parameters into validated JSON data.
- Preserve gameplay balance and behaviour while changing presentation.
- Profile a chaotic effects scene before deciding whether object pooling is
  necessary.
- The Release benchmark with 100 simulated entities and 1,000 particles measured
  about 1.51 ms per frame on the development machine, so do not add object
  pooling in v1.4.0.

Visual direction and production specifications are documented in
`design/VISUAL_BIBLE_v1.4.0.md`.

#### Release verification

- Completed the full manual gameplay and regression checklist for
  `v1.4.0-rc.1`, including the final non-lethal asteroid-hit sound.
- Approved the new backgrounds, sprites, combat effects, enemy behaviour,
  audio changes, and compound ship colliders.
- Built and verified the x64 Debug and Release configurations.
- Validated all JSON data, Visual Studio project files, 4K background sizes,
  removed-resource references, and Release startup.
- Prepared, extracted, file-verified, and smoke-tested the standalone Windows
  release candidate.

### v1.5.0

- Treat v1.5.0 as a complete polish pass over the existing game; defer new
  levels, enemies, bonuses, campaign progression, score tables, and bosses.
- Add a physical-resolution gameplay post-processing pipeline while keeping the
  HUD and crosshair sharp.
- Add true scene bloom, subtle per-level color grading, restrained vignette,
  damage vignette, and localized major-explosion distortion.
- Remove film grain after the visual review showed that it did not improve the
  presentation enough to justify keeping it.
- Add `Post Effects: On / Off` to Graphics settings, and do not add continuous
  motion blur, heavy scanlines, strong lens flares, or readability-reducing
  effects.
- Add a Gameplay settings page with `Screen Shake: On / Off` and
  `Show Score Popups: On / Off`.
- Add Options to the pause menu and reuse the main-menu Options presentation
  without resuming gameplay or its audio.
- Redesign the score HUD with Orbitron, a wide sci-fi frame, `Score: XXXXXX`,
  and a short pulse whenever the score increases.
- Add optional world-positioned `+points` popups with bloom, a 0.5-second upward
  drift, and fade-out.
- Add reusable fade transitions when starting gameplay and for later navigation
  polish.
- Replace the prototype Game Over text with an approved animated screen,
  `Restart Level`, and `Go to Main Menu`; prevent the pause menu from opening
  after death.
- Replace the legacy Level Complete and You Win text with one reusable animated
  result screen, mouse/keyboard/gamepad controls, and fade transitions for
  continuing, replaying, or returning to the main menu.
- Add normalized Xbox and DualSense controller support for twin-stick gameplay
  and every menu, including hot-plugging and dead zones.
- Split Controls into `Keyboard`, `Gamepad`, and `Back`: keyboard bindings remain
  rebindable, while Gamepad shows read-only Xbox and PlayStation layouts in two
  separate blocks.
- Finish with a complete transition/effect tuning pass and controller-inclusive
  regression checklist.
- During the final tuning pass, reduce gameplay-background brightness where
  needed, especially the blue Level 1 background, so cyan player art and shots
  retain clear contrast without losing celestial detail.

#### Release verification

- Synchronized the displayed game version and README for v1.5.0.
- Validated all JSON, referenced assets, Visual Studio project entries, source
  registration, and 4K gameplay-background dimensions.
- Built and verified the x64 Debug and Release configurations.
- Prepared, extracted, file-verified, and startup-smoke-tested
  `v1.5.0-rc.1`.
- Completed the full manual gameplay, menus, settings, transitions, effects,
  and controller-inclusive regression checklist without release-blocking issues.

### v1.6.0 — Campaign foundation

- Add a campaign submenu with Continue Campaign, Start New Campaign,
  Select Level, Horde Mode, Run Mode, and Back to Main Menu. Keep Select Level,
  Horde Mode, and Run Mode visible but disabled in this release.
- Store versioned campaign progress separately from settings under
  LocalAppData, write it atomically, and save only between levels.
- Confirm before replacing existing progress and offer the optional English
  tutorial when starting a new campaign.
- Implement the guided tutorial sequence for movement, firing, asteroids,
  score, armor, enemy combat, collision damage, and the shield pickup.
- Add health and non-stacking timed shield pickups. Picking up another shield
  refreshes it to 100 percent instead of stacking it.
- Verify both pickups during development, then remove forced test drops from the
  completed Level 1.
- Replace the prototype Level 1 flow with three JSON-authored waves and a
  WaveDirector that controls delayed spawns and wave completion.
- Pre-place immediate wave entities before the fade-in, show the synchronized
  First / Second / Final Wave and 3 / 2 / 1 / GO sequence, and animate the
  player ship entering.
- Complete the tutorial into Level 1, preserve the approved result screen,
  and update campaign progress only after a completed level.
- Add `Restart Level` to the pause menu without changing saved campaign progress,
  plus `Restart Tutorial` and `Skip Tutorial` while onboarding is active.
- Add pickup, countdown, and level-complete audio, including temporary music
  ducking and sound-synchronized visual countdown timing.

#### Release verification

- Completed the full campaign, tutorial, wave, pickup, pause, save, audio, and
  input regression pass without release-blocking issues.
- Verified the final x64 Debug and Release configurations.
- Synchronized the displayed game version, README, and release notes for v1.6.0.

### v1.7.0 — New enemies, new pickups, connected levels

- Introduce every campaign level with a dedicated presentation card showing its
  number and authored name before the wave intro.
- Add a ten-second Homing Bullets pickup. Newly fired shots acquire the nearest
  enemy inside a 90-degree aiming cone and steer smoothly toward that target.
- Present active timed bonuses as a dynamic vertical HUD list above armor. Keep
  Shield below Homing Bullets when both are active and close gaps when an effect
  expires independently.
- Add a five-second Time Slowdown pickup. Keep the player and player projectiles
  responsive while slowing enemies, hazards, and hostile projectiles; reinforce
  the effect with audio pitch and a readable post-process treatment.
- Add a two-second controlless cleanup interval after clearing a non-final wave.
  Continue updating projectiles, explosions, particles, sound, and HUD before
  starting the next wave introduction.
- Add JSON-authored deterministic pickup drops, with optional weighted random
  pools reserved for encounters where controlled variation is appropriate.
- Add a rotating symmetric enemy that follows a sinusoidal path and fires three
  projectiles once per second. Configure it to survive ten standard player shots.
- Add a missile carrier and slow, destructible homing missiles. Missiles survive
  three standard shots and explode for 40 damage when touching combat entities.
- Replace the old prototype content with three complete three-wave campaign
  levels. Level 1 uses three large asteroids per wave, Level 2 uses four, and
  Level 3 uses five; all scheduled spawn times are authored from wave start.
- Enable Select Level with a non-scrolling ten-slot layout. Keep locked entries
  disabled and hide their authored names until reached. Replaying an earlier
  level must not rewind campaign progress.
- Expand the level result screen with separate combat score, remaining-armor,
  accuracy, and target-time bonuses, followed by level and campaign totals.
  Keep each level's target time in validated JSON for later balancing.
- Design, but do not yet implement, a separate Salvage Credit economy and
  between-level ship upgrades. Credits should look like rotating pseudo-3D
  luminous tokens and blink before disappearing.

#### Release verification

- Completed the owner-led gameplay and visual verification for all v1.7 systems
  without release-blocking issues.
- Intentionally deferred campaign-wide encounter and pickup balancing until all
  ten campaign levels are implemented; v1.7 keeps every new enemy and pickup
  available for testing.
- Verified the final x64 Debug and Release configurations.
- Synchronized the displayed game version, README, backlog, and release notes
  for v1.7.0.

## Current release

### v1.8.0 — Game Equator

#### Implementation progress

- Completed the first gameplay foundation stage: wave introductions now show
  only `WAVE 1`, `WAVE 2`, or `FINAL WAVE` with a short silent animation. Removed
  the obsolete countdown sound and its audio-balance entry.
- Replaced projectile-count accuracy with attack-count accuracy and introduced
  stable attack identifiers. A successful attack is counted once even when its
  future Triple Shot projectiles or penetrating laser damage multiple targets.
- Applied the approved scoring baseline: 500 points at 75 percent remaining
  armor or better, 600 points for reaching the level accuracy target, and no
  completion-time bonus. Completion time remains an informational statistic.
- Reworked the accepted level-result presentation into distinct Combat and
  Performance statistics without duplicated bonus wording or completion time.
  Added `Restart Level` as a third action; restarting rejects the attempt's
  recovered Parts, score, and best-score update before rebuilding the level.
- Consolidated the result details under one `STATISTICS` heading and removed
  cumulative campaign score from runtime, saves, and UI. Scores are now scoped
  to individual levels and only per-level best results persist.
- Replaced the main-menu `Scores` placeholder with a complete `Records` screen.
  It presents ten permanent campaign level records plus reserved Horde waves,
  Horde score, and Run survival-time records. Records use a separate atomic
  `records.json`, survive starting a new campaign, and import existing campaign
  best scores once when the application starts.
  Keep all Records section headings in the same cyan-white palette as the main
  title and leave clear vertical space around the Run Mode heading.
- Fixed campaign level transitions so every new level restores the player's
  armor to its current maximum instead of carrying damage forward.
- Implemented the finite Parts foundation with authored unique drop IDs,
  five-second gold-pulsing pickups, final-second blinking, and a dedicated
  bottom-right HUD counter for the current attempt with a Parts icon and gold
  collection flash. Collected sources no longer spawn on replay; only missed
  Parts remain visible and recoverable. Levels 1-3 currently contain four Parts
  each; the final Levels 1-9 total will be matched exactly to all upgrade costs
  during the campaign-balance stage.
- Upgraded campaign saves to schema version 3 with a Parts balance, the set
  of collected source IDs. Newly recovered parts stay pending during gameplay,
  are discarded on a failed or restarted attempt, and are persisted only when
  the completed-level result is accepted. The save now also stores the explicit
  campaign phase and four permanent ship-upgrade ranks.
- Added cumulative per-level Parts progress to Level Select and level results.
  Recovering every authored Part on a level awards a 1000-point completion
  bonus.
- Split every Level Select row into a compact level-title button and a separate
  framed Parts counter with the pickup icon, keeping the full row interactive
  for mouse, keyboard, and gamepad navigation.
- Added a dedicated `parts_picked_up.ogg` feedback sound for successful Part
  collection, independently balanced from ordinary pickup audio.
- Implemented the two v1.8 enemy foundations and their approved high-resolution
  sprites. The border laser turret arrives from outside the screen, waits half
  a second, then traverses between neighboring corners with a player-only beam;
  its travel time is read from `enemy_laser_shot.ogg`, the beam has a moving
  geometric spiral, and projectile impacts cannot knock the turret off-route.
  The enlarged shooter station has 200 health and protects each three-second
  creation cycle with its shield while telegraphing the central launch bay with
  `enemy_station_working.ogg`; it starts a rewardless shooter every five
  seconds. Spawned shooters are always rendered above the station. For rapid
  verification, the turret is exposed in Level 1 wave 1 and the station in
  Level 1 wave 2 before the Levels 4-6 encounter pass.
- Refined the enemy encounter routes after hands-on testing. Shooter saucers,
  spinners, and missile carriers now enter toward the inner playfield instead
  of lingering at the screen border; shooters and carriers patrol central
  targets, while spinners preserve their sine motion inside reflected inner
  bounds. The shooter station travels back and forth along either the central
  horizontal or vertical axis, beginning at the midpoint of a boundary.
- Synchronized the station's complete creation sequence to three seconds: its
  shield, attached growing shooter, half-volume working sound, portal, and
  welding sparks share the same window. Long enemy sounds now use per-instance
  playback handles, allowing a destroyed turret or station to stop its own
  active sound immediately; both new enemy sounds are balanced at 50 percent.
- Enlarged the station by another ten percent and replaced its simple outline
  with the player's full shell/glow/hex-grid shield treatment in a hostile red
  palette. Shielded projectile collisions now use the visible shell radius and
  place impact effects directly on its boundary; the station hex grid uses a
  stronger opacity for readability. Its first creation cycle now begins one
  second after entry.
- Added a dedicated station destruction sequence: lethal damage disables the
  station without removing its sprite, runs one second of enlarged localized hull
  explosions, then commits score and drops alongside a much larger final blast,
  stronger camera shake, an escalating sprite shake, and an amplified
  space-distortion shockwave.
- Generated and integrated the v1.8 laser and triple-shot pickup sprites in the
  established armored neon pickup style. Added mutually exclusive ten-second
  weapon modes with a shared HUD timer: triple shot emits one attack as three
  green projectiles at configurable five-degree offsets, while the held-fire laser
  penetrates the full screen and applies standard projectile damage on
  configurable 0.1-second attack ticks. Its beam begins beneath the player,
  carries geometry away from the ship, and uses a higher-pitched three-phase
  sound: ignition once, a sustained middle loop, then the original fade-out when
  the player releases fire. Bonus expiration, wave transitions, player destruction,
  and state teardown stop it immediately. Normal, homing, and triple-shot
  projectiles now use distinct firing pitches. Both player and turret beams use
  a continuous feathered gradient as a cleaner source for the existing scene bloom.
  Existing shield, homing, and time-slow effects remain independently compatible.
- Added provisional Levels 4-6 with exactly three increasingly dense waves each.
  Level 4 formally introduces the laser turret and both weapon pickups, Level 5
  introduces the shooter station, and Level 6 combines every current enemy and
  pickup in the end-of-available-content encounter. Removed all temporary v1.8
  test enemies and guaranteed weapon drops from Level 1. Each new level authors
  four unique finite Part sources, bringing the available Levels 1-6 total to 24.
- Renamed all five 4K gameplay backgrounds from level-specific filenames to
  region-based filenames and assigned them in pairs: Levels 1-2 use the blue
  region, Levels 3-4 use violet, and Levels 5-6 use the asteroid belt. Red and
  deep-void regions remain registered for Levels 7-10. Completing Level 6 now
  returns to Level Select as the end of currently available content and no
  longer triggers the final-campaign Victory / Play Again flow reserved for
  Level 10.
- Fixed inter-wave and inter-level presentation state. The Parts HUD now refreshes
  to zero as soon as the next level is prepared rather than retaining the prior
  value through its introductions. Between non-final waves, the player dissolves
  into a cyan teleport ring, is moved with zero velocity to the protected center,
  and rematerializes before the next enemy deployment. Shooter stations now enter
  from fully outside the screen to the midpoint of their patrol edge before their
  one-second idle period and first construction cycle begin.
- Implemented the first complete Ship Upgrades pass. Accepted campaign results
  now open a dedicated keyboard, mouse, and gamepad screen; leaving it for the
  main menu preserves the pending-upgrades phase, and Continue either starts the
  next level or returns to Level Select after the current content boundary.
  Purchases save immediately and use fixed rank costs `1 / 2 / 3 / 3`, totaling
  exactly 36 Parts across all sixteen upgrades.
- Applied all four functional branches to gameplay: Armor adds 25 percent maximum
  health per rank, Engines add 10 percent acceleration and top speed, Fire Rate
  adds 15 percent firing speed, and Bonus Duration adds one second to every timed
  pickup per rank. The Armor HUD displays the upgraded absolute capacity (`125%`,
  `150%`, `175%`, or `200%`) instead of hiding it behind a normalized full-health
  `100%` label.
- Added a development-only `F1` shortcut under `_DEBUG`. During an active
  non-tutorial level it recovers every still-missing authored Part and opens the
  normal level-complete result, so save acceptance and Ship Upgrades can be
  tested without replaying all three waves. Release builds do not contain the
  shortcut.
- The same Debug shortcut prepares an exact post-result balance of 36 Parts,
  allowing all sixteen upgrade purchases to be verified in one pass regardless
  of the campaign's previous test balance.
- Removed the provisional visual-upgrade implementation after visual review. The
  player again always uses the original `ship_v1_4.png` sprite, while the Ship
  Upgrades screen focuses on four centered functional purchase panels without a
  ship-preview window. Visual ship progression is deferred to a dedicated future
  art pass and does not block validation of the upgrade system.
- Began the approved Ship Upgrades visual pass with a dedicated generated
  deep-space background. It keeps the center dark for interface readability and
  places restrained cyan nebula detail around the outer edges; the asset is used
  only by Ship Upgrades while the remaining interface stays provisional.
- Added the next approved Ship Upgrades visual layer: a generated transparent
  cyan sci-fi header divider, centered behind and beneath `SHIP UPGRADES` as a
  separate high-resolution sprite so it can be positioned independently from
  the title and future Parts panel.
- Moved the header divider fully below the title and replaced the provisional
  explanatory subtitle with a compact Parts balance panel. The panel reuses the
  existing stretch-safe menu-button frame, renders the `PARTS:` label in cyan and
  the live balance in white, and includes a newly generated transparent cyan
  mechanical Parts icon.
- Replaced the temporary two-column upgrade cards with the approved four-row
  vertical foundation. Each row uses a generated minimalist stretch-safe sci-fi
  frame and one of four generated framed cyan icons: Armor shield, Flight Engines
  turbine, Fire Rate triple ammunition, or Bonus Duration clock. Upgrade names
  now use luminous cyan text, while `RANK`, the cyan live rank value, and `/ 4`
  are independent aligned elements. Keyboard and gamepad vertical navigation now
  follows the visible row order.
- Refined the complete interaction layout for the upgrade list. Selected rows use
  dedicated true-gold frame and icon textures plus pulsing bloom instead of
  multiplying gold over cyan, rank values have additional spacing, and each row
  now has a vertical cost divider, raised `COST: X` label, Parts icon, and visual
  `UPGRADE` button. The bottom `Return to Main Menu` and `Continue` controls now
  reuse the minimalist row-frame family and sit symmetrically to the left and
  right of screen center.
- Refresh the Armor HUD immediately after campaign upgrades and maximum health
  are configured, before the first level or wave presentation is rendered.

#### Release verification

- Completed owner-led gameplay and visual verification throughout the staged
  v1.8.0 implementation without remaining release-blocking issues.
- Synchronized the displayed version, README, backlog, project resources, and
  release notes for v1.8.0.
- Completed clean x64 Debug and Release rebuilds for the v1.8.0 release candidate.
- Validated the standalone archive against its package tree, parsed every
  packaged JSON file, and startup-smoke-tested the extracted x64 Release build.

#### Original scope notes

The implementation progress above is the source of truth. The checklist below
is retained only as the original planning record; later hands-on review changed
several details, including Parts lifetime and replay behavior, laser tick timing,
result layout, and removal of cumulative campaign score.

- Replace the repeated `3 / 2 / 1 / GO` wave countdown and its sound with a
  short wave-number presentation. Campaign levels continue to use exactly three
  waves; Level 10 is reserved for the final boss encounter.
- Redesign the level result presentation before implementation so the combat
  score, armor, accuracy, time, collected parts, level total, and campaign total
  remain readable without an excessive wall of text.
- Split results into Combat and Performance columns. Award a fixed 500-point
  armor bonus only at 75 percent remaining armor or better, award a fixed
  600-point accuracy bonus only when the authored level target is reached, and
  keep completion time informational without awarding a time bonus. Do not
  repeat separate bonus-summary rows.
- Add `Continue`, `Restart Level`, and `Main Menu` to completed-level results.
  Restarting rejects the pending result and all newly recovered parts; accepting
  the result commits it before opening upgrades or returning to the main menu.
- Add a border laser turret that travels between the corners of one screen edge
  and sweeps an unobstructed beam across the playfield. The beam interacts only
  with the player.
- Add a slow, heavily armored shooter station with an animated central launch
  bay, a five-second shooter deployment cycle, and alternating three-second
  vulnerable and invulnerable shield phases. Summoned shooters do not grant
  score, pickups, or parts.
- Add a ten-second penetrating player laser that deals one standard-projectile
  hit to every intersected enemy at most once per 0.5 seconds.
- Add a ten-second triple-shot pickup that fires the main projectile plus two
  projectiles at configurable five-degree offsets. Treat the three projectiles
  as one attack for accuracy statistics.
- Make the laser and triple-shot mutually exclusive weapon modes while allowing
  the existing utility pickups to remain independently active.
- Add persistent Parts as a finite campaign resource. Every authored part drop
  has a stable identity: collected drops appear gray on replay and cannot be
  counted again, while missed drops remain collectible. Replaying a level must
  never create additional farmable parts.
- Balance the complete Levels 1–9 campaign so the finite total number of parts
  exactly equals the cost of all sixteen ship-upgrade purchases. Level 10 does
  not drop parts.
- Add a dedicated Parts HUD panel, three-second pickup lifetime, final blink,
  and separate collected-this-level feedback.
- Keep the original player presentation during v1.8 while providing four
  purchasable functional ranks for Armor, Engines, Fire Rate, and Bonus Duration.
- Add a between-level Ship Upgrades state with four functional purchase panels,
  immediate atomic purchase saving, and an entry point from the campaign menu.
  Present fully upgraded panels as non-interactive green `MAX` cards without
  purchase controls, and skip them during pointer and navigation selection.
  Keep the screen's bottom navigation buttons consistent with the main-menu
  button dimensions and presentation.
  Center the live Parts label, value, and icon as one evenly spaced group, and
  keep upgrade-effect descriptions centered within the cards' middle section.
- Track an explicit campaign phase. `Continue Campaign` resumes gameplay while a
  level is active, opens Ship Upgrades while an accepted completion is awaiting
  advancement, and handles the end of currently available content separately.
  Leaving Ship Upgrades through `Return to Main Menu` preserves the pending
  upgrade phase; `Continue` advances to the next level.
- Defer the high-resolution modular ship preview to a dedicated future art pass.
  Build reusable nine-slice panels and tintable interface masks so corners and
  neon borders are not distorted when cards, buttons, and result panels are resized.
- Reset the development campaign save format for v1.8.0; migration of the v1.7
  test save is not required.
- Store permanent score records separately from the active campaign and replace
  the Scores placeholder with a complete keyboard, mouse, and gamepad screen.
- Add Levels 4–6 with three waves each. Keep encounter tuning provisional until
  the complete campaign balance pass in v2.0.
- Reassign the five existing gameplay backgrounds in pairs: Levels 1–2 use the
  blue region, 3–4 the violet region, 5–6 the asteroid region, 7–8 the red
  region, and 9–10 the deep-void region. Rename the assets so their filenames
  describe regions instead of individual levels.
- Treat Level 6 as the end of currently available content rather than the final
  campaign victory; Level 10 remains the only full-campaign ending.
- Generate and approve concepts before implementing the expanded result screen
  and Ship Upgrades screen. Generate and approve the two enemy sprites, two
  pickup sprites, one Parts sprite, and the modular player art during their
  respective implementation stages.
- Build only x64 Debug throughout development. Build Release only after all
  v1.8.0 work is complete and the release candidate is approved.

## Deferred / needs design

- Add controller vibration after the input layer has a dedicated haptics
  backend. SFML 3.1 exposes controller input but no rumble API, XInput covers
  only Xbox controllers, and DualSense needs a separate USB/Bluetooth HID path.
- Add localization with externalized UI text and language selection in Options.
- Review collision behaviour across wrapped screen edges only if the current
  collision style becomes a gameplay problem.
- Decide whether slow motion improves major explosions after the visual-effects
  pass; do not treat it as a committed feature yet.
- Add object pooling only when profiling shows allocation pressure during
  chaotic scenes; do not migrate the game to ECS pre-emptively.
