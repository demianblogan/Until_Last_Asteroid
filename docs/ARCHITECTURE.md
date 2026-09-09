# Architecture

No game engine. An SFML window and a variable-timestep loop drive a stack of
screens; the gameplay screen runs a hand-written simulation that is configured
entirely from JSON. Roughly 33k lines of C++20 across ~95 translation units.

For how the common game-programming patterns map onto this code, see
[PATTERNS.md](PATTERNS.md).

---

## The loop

`main()` sets two things up before anything else:

- exported `NvOptimusEnablement` / `AmdPowerXpressRequestHighPerformance` symbols,
  so hybrid-GPU laptops route the game to the discrete GPU
- a compile-time `static_assert` pinning SFML to exactly 3.1.0

`Application` then:

1. opens the window, applies display settings
2. loads every asset on a background thread while the main thread renders an
   animated loading screen
3. enters the frame loop: `pollEvent` → `stateStack.HandleEvent` →
   `HandleRealtime` → `Update(dt)` → `Render`

`dt` is `min(frameTime, MaxFrameTime)` — variable timestep with a clamp so a
long stall can't tunnel the simulation. The uncapped `frameTime` is kept only
for the FPS counter.

---

## Layers

| Layer | Folder | Responsibility |
|---|---|---|
| App | `src/app` | window, `DisplayManager`, main loop, top-level wiring |
| States | `src/states` | every screen, on a `StateStack` |
| Gameplay | `src/gameplay` | session state, wave/boss direction, tutorial, upgrade math |
| World | `src/core/world` | entity container, split into focused systems |
| Entities | `src/entities` | player, enemies, projectiles, pickups, Parts |
| Rendering | `src/rendering` | background, particles, glow, shields, post-processor |
| UI | `src/ui` | HUD, menu widgets, screens, achievement toast |
| Input | `src/input` | action map, rebinding, gamepad, haptics |
| Core | `src/core` | `Entity` base, `Collision` |
| Services | `src/assets` `src/audio` `src/settings` `src/campaign` `src/records` `src/achievements` `src/localization` | load, persist, translate |

Namespaces: `UI::` for `src/ui`, `Rendering::` for `src/rendering`,
`Haptics::` for the gamepad haptics, `AppDataPath::` / `SafeFileWrite::` for the
file utilities.

---

## State stack

`State` is the interface: `HandleEvent`, `HandleRealtime`, `Update`, `Render`,
`RenderOverlay`. Every state is constructed with a `StateContext` — one struct
of references to every shared service (assets, audio, settings, campaign save,
records, achievements, localization, display, gamepad, haptics).

`StateStack` adds:

- **Deferred changes** — `PushState` / `PopState` / `ClearStates` record a
  `PendingChange`; the stack is only mutated in `ApplyPendingChanges()` at a
  frame boundary, so a state can safely ask to be replaced from inside its own
  `Update`.
- **Transparency** — a state can declare that the state below it still renders
  (pause menu over the frozen game, confirm dialog over options).
- **Caching** — `EnableStateCaching(id)` keeps an instance alive off-stack;
  `OnReactivated()` refreshes it instead of rebuilding.

Screens: company splash → main menu → campaign menu / level select / ship
upgrades / options / records / achievements / credits / language select →
gameplay → pause → campaign complete. The nine front-end screens share a
`MenuState` base plus a `UI::MenuChrome` bundle (background + cursor + fade), so
each screen implements only its own content.

---

## World, split into systems

`World` owns the entity vector and the broad-phase collision pass. Everything
that used to live on it as extra responsibility is now a separate unit it
composes:

| System | Job |
|---|---|
| `ProjectileGeometry` | shot spawn positions, spread, beam shapes |
| `WorldBossHomingTargets` | valid lock-on targets for homing weapons during the boss |
| `WorldCampaignPickupQueue` | the authored order in which campaign pickups drop |
| `WorldRewardExclusionZone` | keeps Parts/bonuses from spawning on top of the player |
| `WorldStatisticsTracker` | per-level kills, shots fired, hits — feeds the results screen |
| `WorldPlayerAttackTracker` | who the player has damaged (accuracy, "untouched" achievements) |
| `WorldEffectEventQueue` | gameplay → rendering effect events (explosions, flashes) |
| `WorldSoundSystem` | positional gameplay sound with voice limiting |

---

## Entities

`Entity` (in `src/core`) is the base: transform, velocity, lifetime, a compound
collider of offset circles. `Enemy` is a shared base for the nine enemy types;
each subclass is just its own movement and firing behaviour. Projectiles
(`Shot`, `HomingMissile`), `Pickup`, `Part`, `Player` and `HelperBot` are
direct `Entity`s.

The boss is not an entity — `BossEncounter` (fight logic, phase state machine,
reinforcement spawning) and `BossVisual` (the ring / diamond / core rendering
and shield animation) drive it as a pair.

---

## Data-driven gameplay

`GameplayData` loads and validates, at startup:

```
assets/data/gameplay/
  player.json     ship stats, colliders, emitter positions
  enemies.json    per-type health, damage, speed, score, fire timing, colliders
  weapons.json    player weapon tuning
  pickups.json    every pickup's effect values and durations
  effects.json    particle / FX parameters
  levels.json     10 levels: background, brightness, target accuracy, waves
  boss.json       all three boss phases
```

Plus `assets/data/achievement_definitions.json` and
`assets/data/audio_balance.json`. Tuning the game is a text edit, not a rebuild.
`WaveDirector` reads a level's wave list and schedules spawns; `GameplaySession`
holds the mutable run state (armor, shield, active timed effects, score, upgrade
ranks).

---

## Rendering

`GameplayState` renders to an off-screen target, then `GameplayPostProcessor`
runs a multi-pass chain: scene bright-pass → Gaussian blur (shared shader) →
composite with colour grading, vignette, damage feedback and explosion
distortion. `NeonGlow` handles UI and projectile bloom. `ParticleSystem` is a
single batched buffer for engines, hits, explosions, smoke, sparks and debris.
Shaders live in `assets/shaders/*.frag`.

---

## Input

`ActionMap` maps abstract actions (Move*, Fire) to bindings; `InputHandler`
resolves them against keyboard, mouse or gamepad. `GamepadManager` normalises
Xbox and PlayStation layouts (different button indices, trigger-as-axis vs
trigger-as-button) and reports which layout is active so the UI can show the
right prompts. `Haptics::GamepadHaptics` drives DualSense rumble, adaptive
triggers and the lightbar through the bundled `DualSenseWindows` library, using
profiles in `VibrationProfiles.h`.

---

## Persistence

All save/config files go through `AppDataPath::Resolve`, which returns
`%LOCALAPPDATA%\Alone Bull Company\Until Last Asteroid\<file>` (or `user_data\`
next to the exe as a fallback):

| File | Owner | Contents |
|---|---|---|
| `settings.json` | `SettingsManager` | graphics, audio, controls, gameplay, language |
| `campaign.json` | `CampaignSaveManager` | level progress, Parts, upgrade ranks |
| `records.json` | `RecordsManager` | per-level scores, Horde / Run bests |
| `achievements.json` | `AchievementManager` | unlocked achievements |

Every write goes through `SafeFileWrite::ReplaceFileAtomically` (write temp →
atomic rename). A file that fails to load or parse is renamed to `<name>.corrupt`
(`.corrupt.1`, `.2`, … if taken) rather than overwritten, so corruption never
silently destroys a save.

---

## Build

Plain MSBuild `.vcxproj`, no CMake or vcpkg.

- Toolset `v145` (Visual Studio 2026), Windows 10 SDK
- `/std:c++20` for Debug, `/std:c++latest` for Release, `/W3`, `/utf-8`
- SFML 3.1.0 headers and libs are expected in `libs/SFML/` (git-ignored) —
  see [`libs/SFML/README.md`](../libs/SFML/README.md)
- `DualSenseWindows` is vendored in `libs/DualSenseWindows/` and built as part
  of the project
- Debug output → `build/`, Release output → `Binaries/` (both git-ignored)
