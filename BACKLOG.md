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

### v1.9.0 — Campaign expansion and survival modes

#### Implementation progress

- Extended the guided tutorial after the shield explanation. A temporary Part
  now appears near the player, respawns if it expires before collection, and
  advances the tutorial only after it is collected. The follow-up message
  explains that Parts purchase between-level ship upgrades and highlights the
  lower-right Parts HUD before the existing tutorial completion message.
- Added forward-compatible continuation for v1.8 campaign saves. Once more
  levels are available, a campaign in the former `content_complete` phase
  advances from its completed boundary to the next level. A save still awaiting
  its between-level upgrade visit advances after that screen instead. The
  existing campaign schema remains valid and `records.json` stays independent.
- Added the friendly invulnerable helper drone as the final campaign pickup.
  Its generated round white-and-navy sprite has a cyan eye and side cannon. A
  level can issue the pickup only once; after collection the drone orbits the
  player through the end of that level and fires a separately tracked homing
  shot at the nearest enemy once per second. Drone hits still award normal
  score and drops but never change the player's accuracy. In Debug builds, F2
  issues the pickup once for isolated testing without changing existing waves.
- Added the Reflector Gunship: a fast figure-eight enemy with 150 health, two
  simultaneous player-targeted shots every 0.5 seconds, and alternating
  three-second vulnerable and reflective-shield phases. Player and helper-bot
  projectiles are returned toward the player with their standard 10 damage;
  continuous player laser fire is blocked rather than reflected. The shield is
  rendered as a red energy shell around the generated dual-cannon sprite. In
  Debug builds, F3 spawns a gunship for isolated combat testing.
- Added Levels 7–9 with exactly three authored waves and four finite Parts each,
  bringing Levels 1–9 to the exact 36 Parts required for all ship upgrades.
  Levels 7–8 use the red-storm region and Level 9 uses deep void. Each new level
  includes the helper bot once in its ordered campaign reward sequence and
  escalates from the new enemy's introduction to dense mixed endgame waves.
- Registered all three gameplay tracks and assigned music by campaign region:
  track 1 for Levels 1–3, track 2 for Levels 4–6, and track 3 for Levels 7–9.
  Pause, game-over, result ducking, time slowdown, restarts, and state exits now
  operate on the active gameplay track instead of assuming track 1.
- Added typed Horde and Run launch modes plus atomic record submission APIs for
  best Horde waves, Horde score, and Run survival time. Their menu entries stay
  disabled until their gameplay directors are implemented and verified.
- Enabled an endless Horde Mode with campaign upgrades and Parts disabled. Its
  level-style introduction presents the mode objective before the first wave.
  Wave 1 contains one large asteroid plus three kamikazes deployed one every two
  seconds, and Wave 2 contains two large asteroids plus three shooters on the
  same cadence. Later waves add one large asteroid per round up to a cap of ten,
  introduce one enemy family per wave until the full roster is active, and add
  one unit to every previously introduced family on each subsequent wave. Horde
  wave titles use unbounded numbering and never display the campaign-only
  `FINAL WAVE` label. The Horde objective uses the existing Exo2 body font for
  improved readability. Exactly
  one randomized bonus is assigned to a defeated large asteroid per wave; the
  shuffled bonus pool avoids repeats until exhausted, and the helper bot can
  appear only once per run. Game Over shows score and survived waves and submits
  both values independently to the permanent records file.
- Horde now selects one of all five gameplay regions at random for each new run.
  Completing every wave grants one uncapped run-only upgrade in a repeating
  Armor, Fire Rate, Engines, and Bonus Duration cycle. Armor rewards expand both
  maximum and current armor, while the active campaign upgrades and save remain
  untouched.
- Enemy homing missiles now count as active wave threats. Killing the final
  missile carrier no longer starts the inter-wave teleport while one of its
  missiles can still reach the player.
- Fixed sustained player-laser audio ownership. A released outro is tracked
  separately from the active loop, replaced when firing resumes, and stopped
  together with the loop when the laser bonus expires, control is disabled, or
  the player is destroyed.
- Increased the authored gameplay-background image brightness globally by ten
  percent, covering the tutorial, every campaign level, Horde, and future modes
  that use the shared gameplay background renderer.
- Enabled Run Mode without player firing, Parts, campaign upgrades, or discrete
  waves. One randomly directed and paced large asteroid appears every ten
  seconds, capped at ten total. Shooters appear every fifteen seconds, capped at
  three total. A single Laser Turret appears at 40 seconds and a single
  Reflector Gunship at 60 seconds. No other enemies or pickups appear.
  Any damage reaching armor immediately ends the run, while shield-absorbed
  impacts remain survivable. The upper-left HUD is a live `MM:SS` stopwatch
  instead of a score panel, matching Game Over and Records; the Parts panel and aiming cursor are
  hidden, and Game Over shows both the run time and permanent best time.
- Hid the campaign-only Parts HUD panel in Horde while retaining it in campaign
  levels and the tutorial.
- Corrected the long-standing wave-title animation target so `WAVE N` and
  `FINAL WAVE` settle at the vertical center of the playfield before fading.
- Kept campaign upgrades disabled in Horde and Run so their records remain
  independent from the active campaign save.
- Rebalanced all 27 campaign waves for the final Levels 1-9 tuning pass. Each
  level now introduces its intended cumulative bonus sequence and retains all
  36 permanent Part identifiers.
  Levels 1-7 follow the approved absolute spawn schedule; Levels 8-9 use five
  and six large asteroids per wave respectively and escalate through denser
  mixed formations of every endgame enemy family.
- Campaign rewards are now distributed across randomly selected enemy spawn
  slots for the whole level. Asteroids and the first enemy cannot carry Parts;
  Part and bonus slots never overlap. Level N grants exactly N bonuses: the new
  bonus is first on Levels 1-7, followed by previously introduced types, while
  Levels 8-9 continue cyclically after the helper bot. Existing collected Parts
  are omitted without changing their permanent IDs. Bonus-bearing enemies are
  selected independently inside each wave using the fixed distributions 0/1/0,
  1/1/0, 1/1/1, 2/1/1, 2/2/1, 2/2/2, 3/2/2, 3/3/2, and 3/3/3 for Levels 1-9.
- Temporary bonus timers now freeze as soon as a campaign or Horde wave is
  cleared and remain frozen through teleportation and the next-wave intro.
- Shooter Stations now visibly fly in from outside the arena. Simultaneous
  stations receive distinct parallel routes and cannot spawn on top of one
  another; off-screen arriving stations are exempt from screen wrapping.
- Simultaneous Laser Turrets now receive distinct perimeter routes and remain
  exempt from screen wrapping until their entrance is complete, preventing
  turrets in the same spawn group from occupying one position.
- Shooter Stations now enter with their shield already active and deploy their
  first shooter immediately upon reaching the arena, removing the unprotected
  opening before their normal production cycle begins.
- Completing a previously unlocked level from Level Select now opens Ship
  Upgrades after banking newly collected Parts. Back and the primary return
  button lead back to Level Select without changing campaign progression.
- Level 9 now grants four bonuses in every wave. Armor restoration is guaranteed
  once per wave, with the remaining three rewards continuing the ordered bonus
  rotation.
- Added a dedicated `player_laser_shot.ogg` resource for the player's sustained
  laser while retaining `enemy_laser_shot.ogg` for enemy Laser Turrets.
- Batched each energy shield's complete hexagonal grid into one draw call and
  reduced circular shell tessellation from 96 to 64 points. This removes the
  hundreds of per-frame draw submissions previously caused by every active
  Shooter Station shield while preserving its animated layered appearance.
- Restored the complete Main Menu -> Campaign Menu -> Level Select hierarchy
  after finishing a replayed level and visiting Ship Upgrades. Returning through
  both menus no longer empties the state stack and leaves a permanent black
  screen. The application now also closes safely if any future route exhausts
  the state stack.
- Large and small asteroids now start at a random orientation and rotate around
  their center with randomized clockwise or counter-clockwise angular velocity.
  Their authored base rotation speeds are stored in `enemies.json`; movement,
  circular collision, fragmentation, and combat balance remain unchanged.
- Shooters, Missile Carriers, and Reflector Gunships now turn toward the player
  along the shortest arc at individually authored speeds instead of snapping
  every frame. Shooter and Reflector volleys follow the ship's current forward
  direction, making rapid player movement capable of throwing off their aim;
  homing missiles retain their own independent guidance after launch.
- Added four generated 3840x2160 gameplay backgrounds: emerald aurora, rose
  stellar nursery, frozen silver expanse, and yellow-lime ion storm. Campaign
  Levels 1-9 now each use a unique background, matching gradient/star colors,
  and an individual post-process palette. Horde randomly selects from all nine.
- Kept all pre-existing texture resource identifiers numerically stable by
  appending the four new background keys to `Config::Texture`. A full Debug x64
  rebuild removed mixed stale objects that briefly mapped backgrounds onto UI
  and gameplay sprites after an interrupted compilation.
- Built the approved v1.9.0 Release candidate with a clean x64 Release rebuild:
  zero compiler warnings and zero errors. The displayed game version is v1.9.0.

#### Release verification

- Owner-led gameplay verification completed throughout staged development,
  including the full campaign, Horde Mode, Run Mode, replay upgrades, final
  balance, new enemies, pickups, backgrounds, audio, and performance fixes.
- Parsed every source gameplay and audio JSON file successfully before packaging.
- Full Windows x64 Debug and Release builds completed successfully.
- Built and extracted `UntilLastAsteroid-v1.9.0-win64.zip`, verified every
  packaged JSON file and required runtime asset, and passed a five-second
  startup smoke test with the extracted Release executable. Archive SHA-256:
  `B7C5304C9B5006F262BF7DD5DE37E2E9BBFBD7CEFA8EA1007F7C7E064F3E8165`.

## In development

### v2.0 — Final completion

#### Approved scope and implementation rules

- Add Level 10, titled `THE LAST HORIZON`, as the campaign's wave-free final
  boss encounter. Levels 1-9 continue to contain exactly three waves.
- Implement the final boss as a dedicated encounter state machine instead of
  extending `WaveDirector`. Keep boss timing, health, damage, spawn intervals,
  and phase thresholds in validated gameplay data.
- Give the boss three exact health ranges: outer ring from 100 to 70 percent,
  four inner teleporters from 70 to 30 percent, and the exposed core from 30 to
  zero percent. Award exactly 10,000 score when the boss is destroyed.
- Boss reinforcements do not award score or Parts, but may drop bonuses to help
  the player. Keep score, pickup, and Part reward permissions independent.
- Do not show wave titles or the normal level-results table during Level 10.
  Finish the destruction cinematic before opening the campaign-completion
  screen.
- Preserve v1.8/v1.9 schema-3 campaign saves through an explicit migration.
  Migrated campaigns retain progress, Parts, upgrades, and scores, but only a
  campaign started in v2.0 is eligible for the no-death achievement.
- Keep `records.json` independent and unchanged when starting a new campaign.
  Store permanent achievement unlocks separately from the active campaign.
- Localize all player-facing text into English, Spanish, Russian, Ukrainian,
  and Arabic. Language changes are immediate. The owner will provide suitable
  fonts before the localization-font stage. Complete and approve the entire
  English version first; localization is a later v2.0 stage.
- Add nine achievements. Remove the proposed no-upgrades achievement. Use a
  provisional target of three minutes for Run Mode and ten completed waves for
  Horde Mode, subject to final balance testing.
- Add localized Credits and consistent repository authorship for Demian Blogan.
- Build only x64 Debug during implementation. Build x64 Release only after the
  complete version is approved for release-candidate preparation.
- After every implementation stage, report the exact changes and verification
  result, then wait for owner review before continuing.

#### Stage 1 — Technical contracts

- Added an explicit `waves` / `boss` encounter kind to gameplay level data.
  Wave encounters require exactly three waves; only Level 10 may be a boss
  encounter, and a boss encounter must contain no waves.
- Advanced campaign saves to schema 4 while accepting schema-3 v1.8/v1.9
  files. New v2.0 campaigns begin eligible for the no-death achievement;
  migrated campaigns remain compatible but are not retroactively eligible.
- Split enemy reward permission into score, pickup, and Part controls so Level
  10 reinforcements can drop helpful bonuses without granting score or Parts.

#### Stage 2 — Final-boss visual concept

- Approved the top-down assembled silhouette and created three separate RGBA
  production sprites: the mechanical brain and obelisk core, the diamond frame
  with four working teleporters, and the outer ring with six integrated guns.
- Match the existing dark-metal, red-emissive enemy art direction. Keep the
  orange retaliatory shield as a separate runtime effect rather than baking it
  into the boss sprites.
- Keep the modules independently scalable in gameplay so their assembled size
  can be tuned against the 1920x1080 playfield without resampling source art.

#### Stage 3 — Level 10 presentation foundation

- Added `THE LAST HORIZON` to gameplay data as the only wave-free boss
  encounter and added its dedicated red-black final-space background.
- Added a dedicated `BossEncounter` presentation component. After the level
  title, the player materializes, `boss_fight.ogg` starts, and the assembled
  three-layer boss enters from above over four seconds under a pulsing orange
  shield. It then holds the shield for two seconds before reaching the future
  Phase 1 ready state.
- Kept player firing disabled for this presentation-only stage. Damage,
  retaliatory lightning, boss health, and Phase 1 combat belong to the next
  reviewed stage.
- Presentation review: reduced the diamond from 505 to 455 logical pixels and
  the core from 330 to 280 pixels to separate all three silhouettes. Anchored
  the core explicitly at source coordinate `(628, 628)`, hid the Parts HUD on
  Level 10, and replaced the temporary circle with the shared hex-grid enemy
  energy shield rendered in an orange palette.
- Follow-up visual alignment: shifted only the core sprite 37 logical pixels
  upward so the circular brain, rather than the full brain-and-obelisk image
  bounds, is centered inside the diamond.

#### Stage 4 — Intro shield retaliation

- Enabled player movement and firing during the boss arrival. Player bullets
  now collide with the orange shield surface, count as accurate hits, disappear
  on impact, and produce the normal shield-impact feedback.
- Each shield impact creates a short jagged orange lightning bolt from the exact
  impact point to the player's current position and applies 10 damage through
  the normal player shield/health pipeline. Existing damage invulnerability
  prevents overlapping bullets from multiplying damage in a single instant.
- The retaliatory shield remains active during the four-second arrival and the
  two-second post-arrival suspense delay only. Phase shield cycles will reuse
  this behaviour in the combat stages.

#### Stage 5 — Phase 1 base combat

- Added validated `boss.json` tuning for total health, the Phase 1 end ratio,
  ring collision radii, rotation speed, cannon orbit, fire interval, stagger,
  and the exact six-cannon count.
- When the intro shield drops, a long red `BOSS ARMOR` bar appears, the outer
  ring rotates at 8 degrees per second, and its six guns fire in a repeating
  sequence: each gun fires every 0.6 seconds and adjacent guns are offset by
  0.1 seconds.
- Player bullets damage only the annular outer-ring hit region. The inner
  diamond and core remain protected, and health is clamped at 70 percent until
  the Phase 1 destruction transition is implemented.
- Phase 1 visual review: replaced the stretched source art with new symmetric
  production sprites: a mathematically circular outer ring and an equal-sided
  diamond. The ring is assembled by repeating one mirrored 60-degree sector,
  so all six rail segments and guns are rotationally identical. Both sprites use
  uniform runtime scaling, eliminating rotational squeeze and wobble. Guns fire
  strictly along their outward radial axes from the six equidistant muzzle
  centers. Ring collision feedback is projected onto its visible 264-pixel rail
  boundary, successful hits trigger the standard enemy white flash,
  and the bar label displays `Boss Armor: XX%`.

#### Stage 6 — Phase 1 shield cycles and ring destruction

- Clamp each exposed damage window at exactly 90, 80, and 70 percent boss
  armor. At the first two thresholds, restore the retaliatory orange shield for
  ten seconds while the ring keeps rotating and all six guns keep firing.
- During the first shield cycle, spawn one kamikaze every two seconds. During
  the second, also spawn one shooter every three seconds. Boss reinforcements
  award no score or Parts. Phase 1 guarantees exactly one health pickup and one
  homing-bullets pickup, distributed as one reward in each shield cycle instead
  of attaching both rewards to the first reinforcements.
- At 70 percent, stop the ring and its guns, shake it under repeated small
  explosions for two seconds, then remove it with a large explosion. Leave the
  diamond and core ready for the separately reviewed Phase 2 implementation.
- Before each combat shield cycle, blink its orange projection for 1.5 seconds
  while the ring is damage-clamped but retaliation remains disabled. This lets
  already-fired bullets expire safely and warns the player to release fire.
  During both the warning and active shield, keep enemy ships completely outside
  the shield volume, including their collision radius and an additional margin.

#### Stage 7 — Phase 2 teleporter combat

- Rotate the inner diamond continuously and treat its four portals as separate
  300-HP targets. Diamond walls absorb player fire without taking damage; each
  destroyed portal removes exactly ten percent of total boss armor and receives
  an immediate explosion plus a dark destroyed-state overlay. Measure each
  portal center from the production sprite rather than assuming a perfectly
  symmetric orbit, and reuse those aligned positions for collision, destroyed
  masks, reinforcement origins, and player homing-bullet targets.
- Cycle portal deployments in vertex order: kamikaze, shooter, spinner, and
  missile carrier. Phase 2 does not spawn reflector gunships. Spawn immediately
  from the first portal, then every four, three, two, or one seconds according
  to the number of surviving portals. Add one ordinary edge-spawned shooter
  every three seconds throughout Phase 2.
- Guarantee exactly four Phase 2 rewards in order: shield, helper bot, health,
  and triple shot. Unlock one reward at each surviving-portal tier so they are
  spread across the phase instead of all dropping near its start. Keep all later
  reinforcements reward-free. Increase the helper bot's global firing rate from
  one shot per second to one shot every 0.5 seconds so the Phase 2 reward remains
  useful under boss-level pressure.
- After each of the first three portal destructions, use the shared 1.5-second
  shield warning followed by five seconds of retaliatory orange shielding while
  the diamond and surviving portals continue operating. After the fourth portal,
  stop the diamond, shake it under small explosions for two seconds, then remove
  it in a large explosion and leave the core ready for Phase 3.
- Use a 225-pixel diamond shield instead of retaining the 355-pixel ring shield.
  Keep the portal origin for teleport deployments, permanently exclude inward-
  moving enemies from the current boss body, and relocate any reward that would
  otherwise land inside that exclusion volume. Hide both Score and Parts panels
  on Level 10. Add scaled muzzle flashes to all six ring cannons and additive
  orange-white neon bloom to retaliatory lightning.

#### Stage 8 — Phase 3 core combat

- After the diamond is destroyed, leave only the mechanical brain and start a
  full-screen obelisk beam rotating clockwise. The beam damages the player on
  contact, renders above the brain sprite, and accelerates at each ten-percent
  armor threshold. Use phase speeds of 14, 22, and 32 degrees per second. Match
  the existing laser-turret beam language with a wider feathered core, additive
  glow, and moving energy markers, recolored orange for the obelisk.
- Protect the core with an orange shield and place four laser turrets in the
  arena corners. Aim them clockwise along the arena edges—right, down, left,
  and up—to form a rectangular laser boundary. Destroying all four turrets
  removes the shield and exposes the core. At 20 and 10 percent armor, use the
  shared 1.5-second shield warning before restoring the shield and a fresh set.
- At 20 percent armor, begin spawning one large asteroid per second from the
  arena boundary. Give each asteroid an inward velocity plus a visible teleport
  materialization effect instead of letting it pop into view. At 10 percent,
  keep the asteroid pressure and have four shooter stations simultaneously move
  slowly inward from their corresponding screen edges to the side midpoints,
  then remain there. Clamp core damage at 20, 10, and zero percent so every
  defensive cycle must be completed.
- Add two Phase 3 rewards in separate escalation cycles: laser on the first
  laser turret after 20 percent, then time slowdown on the first laser turret
  after 10 percent. Phase 3 asteroids never carry guaranteed rewards.
- Treat every visible boss body and active shield as a solid contact boundary.
  Push the player outside the current ring, diamond, core, or shield radius and
  apply the standard invulnerability-aware enemy contact damage. Center the
  Phase 3 shield on the visual brain and reduce it to a 140-pixel radius.
- Update mid-combat materialization continuously, not only while a wave intro is
  active, so Phase 3 asteroids fade into view instead of remaining invisible.
- In Debug x64, use F2 to advance from the outer ring to the diamond and from
  the diamond to the core, clearing active projectiles at each transition. Keep
  this testing shortcut excluded from non-Debug builds. Remove the former F1
  level-completion and F3 reflector-spawn shortcuts.
- Keep the final destruction cinematic, HUD shutdown, 10,000-point boss reward,
  campaign victory transition, and completion screen in the next separately
  reviewed stage.

#### Stage 9 — Boss destruction sequence

- Route each player-laser damage tick into the exposed core collision and reuse
  the same 20, 10, and zero-percent damage clamps, hit feedback, and phase
  transitions as ordinary player projectiles.
- Spawn every core laser turret fully outside its corresponding screen edge and
  move it to a stationary corner destination. Extend the obelisk beam 180 pixels
  beyond its screen intersection so its flat geometry edge is never visible.
- At zero armor, stop the boss beam and combat music, disable player firing,
  crosshair, HUD, pickups, companions, and temporary bonuses while preserving
  player movement. Award the fixed 10,000-point boss score once.
- Destroy remaining enemies, missiles, and asteroids one at a time at 0.1-second
  intervals with their effects and rewards disabled. Cover the core with small
  explosions for three seconds, then hide it behind the largest boss explosion
  and leave the player alone in space for three seconds.
- Make the player cinematically invulnerable as soon as the sequence starts.
  Include active and pending hostiles in cleanup, suppress meteor fragments, and
  force-clear any remainder before the three-second solitude shot.
- Shake the camera for one second during ring and diamond destruction. Start a
  continuous 3.5-second shake when core destruction begins, covering the full
  three-second cascade and the half-second following the final explosion.

#### Stage 10 — Campaign victory transition

- After the solitude shot, fade out over 1.8 seconds while starting the supplied
  non-looping `campaign_victory.ogg` track. Do not show Level 10 statistics.
- Present a dedicated English campaign-completion message with the requested
  Horde Mode, Run Mode, and achievements postscript plus a single `Thank You`
  button. The button persists Level 10 completion and its 10,000-point boss
  score, then returns directly to the main menu.
- Replace the temporary results overlay with a dedicated state over the animated
  main-menu background. Render separate generated transparent title and body
  frames through reusable nine-slice geometry, highlight the continuing-journey
  message, reveal the modal smoothly, and activate its single button only after
  the entrance animation. Preserve the victory track across this transition.
- Keep panel shading strictly inside each frame's transparent opening. Preserve
  nine-slice corner proportions, give the body copy enough horizontal room,
  and use one consistent Exo2 body style with `Good Luck!` on its own line.
- Replace the transparent-frame-plus-shade composition entirely with generated
  nine-slice panels whose dark opaque center is part of each source texture.
  Keep only the exterior transparent, with self-contained corner slices and
  straight stretchable edge segments.
- Increase completion-panel copy by approximately 1.3x and center every line
  independently; SFML's multiline text bounds do not provide paragraph-level
  center alignment by themselves.
- Reset the completed-campaign level-select transition after its single push so
  returning from Level Select cannot reveal duplicate stacked copies.
- In Debug x64, allow F1 to begin the final boss defeat sequence immediately so
  the completion transition and screen can be reviewed without replaying combat.

#### Stage 11 — Achievement foundation

- Define nine stable achievement identifiers and external English metadata in
  `assets/data/achievements.json`, including display order, provisional Run and
  Horde thresholds, descriptions, and future icon paths.
- Store permanent unlocks independently in Local AppData `achievements.json`.
  New campaigns must not clear this file or `records.json`. Use versioned JSON,
  atomic temporary-file replacement, rollback on failed writes, deterministic
  serialization, and preservation of corrupt files.
- Add an application-owned `AchievementManager` and expose it through
  `StateContext`. Validate that all nine definitions and display positions are
  present exactly once. Gameplay conditions, notifications, icons, and the 3x3
  menu remain in the next separately reviewed stages.

#### Stage 12 — Achievement conditions and presentation

- Evaluate all nine achievements at authoritative gameplay events: Levels 1,
  5, and 10 completion; 180 live Run Mode seconds; ten completed Horde waves;
  all four upgrade categories at rank four; a persisted tutorial-skip choice;
  a new schema-4 campaign completed without a death; and Level 10 completed
  without accepted shield or health damage.
- Persist loss of no-death eligibility immediately on a main-campaign death.
  Ignore tutorial, selected-level, Horde, and Run deaths. Reset per-level damage
  tracking on level start and restart. Suppress boss completion achievements for
  any Debug run that used F1 or F2.
- Generate nine square production icons and load them as normal texture assets.
  Queue each new unlock in the application-owned manager and display a global
  five-second slide-down notification that survives state transitions. Reuse
  `level_complete.ogg` until a distinct achievement sound is present in assets.
- Add an English `Achievements` main-menu entry and dedicated animated-menu-
  background state. Present nine non-interactive tiles in a 3x3 grid, with gray
  locked art and normal gold/cyan treatment for unlocked achievements, plus one
  controller-compatible `Return to Main Menu` button.

#### Stage 13 — Credits

- Add `Credits` as a sixth main-menu entry and reflow all menu buttons to fit
  the 1920x1080 logical viewport without overlap. Place it below `Options`.
- Present a dedicated Credits state over the animated main-menu background.
  Reuse the opaque campaign-completion nine-slice panel, centered Exo2 copy,
  and the standard glowing menu treatment.
- Credit only `Demian Blogan`. Include the supplied contact address
  `demianblognan@gmail.com`, the `Blogan Programming` YouTube channel, and the
  `github.com/demianblogan/Until_Last_Asteroid` source repository. Provide one
  mouse-, keyboard-, and controller-compatible `Return to Main Menu` button.
- Pass bloom render states through every nine-slice segment so the panel and its
  glow share the same render-texture origin instead of appearing offset.

#### Stage 14 — Localization foundation

- Added a versioned localization section to `settings.json` with stable `en`,
  `es`, `ru`, `uk`, and `ar` identifiers plus a separate first-run selection
  flag. Existing settings migrate to English without losing any options.
- Registered Noto Sans regular/bold for Latin and Cyrillic UI and Noto Sans
  Arabic regular/bold for Arabic UI. Retain Exo 2 Regular for existing body
  styling and remove only its seventeen unused bundled variants.
- Added five validated UTF-8 JSON catalogs with identical key contracts and a
  central `LocalizationManager` that provides English fallback, native language
  names, language persistence, and RTL metadata.
- Added a controller-, keyboard-, and mouse-compatible first-run language screen
  after the company splash. A successful selection is saved atomically before
  entering the main menu, so the screen appears only until a language is chosen.
- Fixed first-run input explicitly: accept clicks on any language row, handle
  arrows/W/S plus Enter/Space directly, and support
  both the controller D-pad and left stick for menu navigation.
- Keep the native cursor hidden and render the shared glowing game cursor at the
  actual mouse position; use the selected-row pointer only for controller input.
- Separate menu-button frame and label opacity. This restores character-by-
  character main-menu typing while preserving the campaign-completion button's
  intentionally unified fade-in.
- Convert the main-menu intro typer from byte-counted `std::string` slices to
  Unicode code-point slices, so accented Spanish and Cyrillic never expose a
  partial UTF-8 sequence during the character animation.
- Add a dedicated `Language` page to Options with all five native language
  names. Persist changes immediately, refresh the current Options typography,
  and notify the already-open main menu to replace both its font and labels
  without restarting or rebuilding the state stack.
- Add an internal Arabic presentation pass for joined contextual letter forms
  and visual RTL line order, using the supplied Noto Sans Arabic fonts. Compile
  x64 source files explicitly as UTF-8 so Arabic literals remain deterministic.
- Preserve the chosen language when `Restore Defaults` resets graphics, audio,
  gameplay, and controls; resetting ordinary options must never reopen the
  first-run language screen.
- Localize the campaign menu, overwrite/tutorial dialogs, Level Select, all ten
  campaign level titles, Pause, and Records. Keep gameplay level JSON as the
  authoritative structural data while resolving player-facing level names by
  stable level-number keys in each language catalog.
- Centralize regular/bold localized font selection so English retains the
  established Orbitron/Exo presentation while Spanish and Cyrillic use Noto
  Sans and Arabic uses Noto Sans Arabic consistently.
- Complete the five-language pass across every Options subpage and display
  dialog, ship upgrades, achievements and unlock toasts, Credits, campaign
  completion, gameplay HUD, boss armor, level/wave introductions, Horde and
  Run objectives, Game Over, level results, and the full tutorial sequence.
- Localize the displayed game title and preserve the original English title as
  the English catalog value. Keep external URLs, the e-mail address, and the
  permanent `Demian Blogan` credit intact in every language.
- Add width-aware typography for shared menu buttons and constrained Options,
  achievement, Credits, completion, intro, and tutorial text. Long Spanish,
  Cyrillic, and Arabic strings reduce their character size within safe minimums
  instead of extending beyond their panels.
- Preserve left-to-right ASCII runs such as scores, percentages, URLs, e-mail
  addresses, and version numbers inside the Arabic visual-order shaping pass.
- Validate all five catalogs as JSON and enforce an identical 212-key contract
  at startup, with English fallback retained for missing runtime lookups.

#### Stage 15 — Final polish and profiling

- Remove the menu-transition stalls caused by eagerly building several
  full-resolution blur chains. Render bloom at reduced internal resolution and
  cache the static controller-layout page instead of rebuilding it every frame.
- Start normal-wave asteroid materialization together with the asteroid spawn,
  including while the `Wave X` introduction is still active.
- Keep the player's laser loop alive for the entire held-fire interval and stop
  it immediately when firing ends.
- Remove proven dead state scaffolding, duplicate UI APIs, unused resource IDs,
  obsolete resource accessors, an unused localization key, and superseded or
  unreferenced legacy assets. Keep dynamically loaded data and user files
  untouched unless their lack of use is demonstrated.
- Profile and review remaining runtime hot paths before making architectural
  optimization changes. Evaluate threading separately; do not add concurrency
  unless measured work can be isolated safely from SFML graphics and game state.
- Replace the blank native startup window with a responsive dark loading screen,
  staged progress, and close/resize event handling while assets, localization,
  achievements, and save-backed systems initialize on the render thread.
- Remove Credits' full-panel shader blur and replace it with two cheap aligned
  nine-slice border halos. Raise the shared bloom resolution from 35% to 50%
  while reducing its outer iterations, improving Full HD quality without
  restoring the Credits transition stall.
- Resolve the runtime asset root from the executable location before settings,
  saves, audio, and rendering systems are constructed. Direct Debug/Release
  executable launches must not depend on the shell or IDE working directory.
- Remove all lazy shader bloom and the decorative frame halo from Credits and
  Achievements. Their return buttons now start idle and use the shared menu
  button selected texture only after mouse/controller navigation, eliminating
  first-entry stalls and the false initial hover state.
- Let a selected-level completion advance the campaign without reopening an
  obsolete unfinished tutorial flag from an older save. Tutorial recovery is
  now limited to campaigns that have not completed any level yet.
- Redesign startup loading as a black screen with a near-full-width bottom bar,
  a five-language `Loading` label and a slow opacity pulse. Repaint only at
  coarse loading stages: per-resource `display()` calls were measured at up to
  73 seconds for textures and must not serialize every GPU upload.
- Disable VSync/frame limiting only during startup, then restore the configured
  values before the normal game loop.
- Register the ten 4K gameplay backgrounds for lazy first-use loading. They
  account for roughly 316 MiB decoded and are not needed by startup or menus;
  the main-menu background remains eager.
- Replace point-by-point localized text fitting with a proportional size jump
  implemented as reuse-preserving text scaling. Text-heavy screens no longer
  rasterize the same glyphs at many intermediate font sizes; Credits uses one
  body-font atlas while retaining its designed visual size hierarchy.
- Cache dropdown `sf::Text` objects for the lifetime of an open Options list.
  Profiling reduced steady dropdown rendering from about 1.35 seconds per frame
  to 0.23-2.87 milliseconds. Remove shader bloom from dropdown rows.
- Replace the cursor's five-render-target blur chain with two small additive
  sprite halos. A cursor no longer allocates a full `NeonGlow` instance for
  every newly opened state.

## Deferred / needs design

- Add controller vibration after the input layer has a dedicated haptics
  backend. SFML 3.1 exposes controller input but no rumble API, XInput covers
  only Xbox controllers, and DualSense needs a separate USB/Bluetooth HID path.
- Review collision behaviour across wrapped screen edges only if the current
  collision style becomes a gameplay problem.
- Decide whether slow motion improves major explosions after the visual-effects
  pass; do not treat it as a committed feature yet.
- Add object pooling only when profiling shows allocation pressure during
  chaotic scenes; do not migrate the game to ECS pre-emptively.
