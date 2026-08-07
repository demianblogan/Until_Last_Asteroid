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

## Planned — v1.1.0

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
- Use the existing `trs_million` font until the later UI/UX art pass.
- Reserve the lower-left for menu controls and use a layered space-and-asteroid
  parallax background.
- Add an explicit paused game state.
- Pause and resume active gameplay with Escape.
- Pause automatically when the window loses focus or is minimized.
- Disable movement and shooting while paused.
- Add a pause overlay or menu.
- Resume without a large frame-time jump.

## Deferred / needs design

- Add localization with externalized UI text and language selection in Options.
- Add controller support, including skipping the company splash with a gamepad.
- Review collision behaviour across wrapped screen edges only if the current
  collision style becomes a gameplay problem.
- Decide whether slow motion improves major explosions after the visual-effects
  pass; do not treat it as a committed feature yet.
- Add object pooling only when profiling shows allocation pressure during
  chaotic scenes; do not migrate the game to ECS pre-emptively.
