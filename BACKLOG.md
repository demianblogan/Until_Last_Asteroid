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

## Planned

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
