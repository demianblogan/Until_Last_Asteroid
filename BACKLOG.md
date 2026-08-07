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

## Deferred / needs design

- Add localization with externalized UI text and language selection in Options.
- Add controller support, including skipping the company splash with a gamepad.
- Review collision behaviour across wrapped screen edges only if the current
  collision style becomes a gameplay problem.
- Decide whether slow motion improves major explosions after the visual-effects
  pass; do not treat it as a committed feature yet.
- Add object pooling only when profiling shows allocation pressure during
  chaotic scenes; do not migrate the game to ECS pre-emptively.
