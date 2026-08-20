# 🚀 Until Last Asteroid (C++ / SFML)

A fast-paced top-down space shooter built with **C++23** and **SFML 3.1.0**.

Destroy asteroids, fight enemy ships, survive waves, and complete all levels.

---

## 🎮 Gameplay

![Preview](https://github.com/user-attachments/assets/1c74a1c9-13ed-4ca6-a618-89a87a0a5df7)

---

## 📦 Download & Play

👉 [Download Latest Release](../../releases)

Quick start:

1. Download `.zip` from Releases
2. Extract it
3. Run `UntilLastAsteroid.exe`

---

## 🕹 Controls

Default controls (all bindings can be changed in Options):

| Action        | Input              |
|--------------|-------------------|
| Move         | W A S D           |
| Aim          | Mouse             |
| Shoot        | Left Mouse Button |
| Pause        | ESC               |

Xbox and PlayStation-style controllers are also supported with twin-stick
movement and aiming, trigger shooting, and D-pad menu navigation.

---

## 🧠 Game Features

- Campaign menu with persistent, versioned progress
- Optional guided tutorial for movement, combat, scoring, armor, pickups, Parts,
  and ship upgrades
- Nine connected JSON-authored campaign levels with animated introductions
- Endless Horde Mode with escalating waves and run-only cyclic upgrades
- One-hit Run Mode with survival-time records and restricted defensive pickups
- Finite collectible Parts and four functional ship-upgrade branches
- Permanent per-level Records independent from campaign save progress
- Health, shield, homing-bullet, time-slowdown, laser, triple-shot, and helper-bot
  pickups with dynamic HUD feedback
- Dynamic enemy waves system
- Score system with scaling difficulty
- Player physics (acceleration, damping, max speed)
- Screen wrapping (Asteroids-style world)
- Sound effects and background music
- Health, damage, knockback, invulnerability, and score systems
- HUD with score and a color-changing health bar
- Nine unique 4K space backgrounds with animated star and dust parallax
- High-resolution player, enemy, asteroid, and projectile artwork
- Batched particles for engines, hits, explosions, smoke, sparks, and debris
- Projectile glow, hit flashes, camera shake, and compound ship colliders
- Configurable gameplay post-processing with bloom, color grading, vignette,
  damage feedback, and explosion distortion
- Animated score HUD, optional score popups, and polished Game Over, level
  completion, and victory screens
- Skippable company splash screen
- Animated sci-fi main menu
- Pause menu with Resume, Restart Level, Options, and Back to Main Menu
- Persistent Graphics, Audio, and Controls settings
- Fullscreen, Windowed, and Borderless display modes
- Rebindable keyboard and mouse controls
- Xbox and DualSense controller layouts with automatic input switching
- Gameplay options for screen shake and score popups
- Bloom-highlighted UI, menu cursor, and gameplay crosshair

---

## 👾 Enemies

### 🪨 Meteors
- **Large** → slow, splits into two small asteroids (+100 pts)
- **Small** → faster and does not split (+50 pts)

### 🚀 Saucers
- **Kamikaze** → spins while aggressively chasing the player (+200 pts)
- **Shooter** → alternates fire between two cannons (+250 pts)
- **Spinner** → follows a sinusoidal path while rotating and firing in three directions
- **Missile Carrier** → launches slow, destructible homing missiles
- **Laser Turret** → patrols a screen edge with a player-only sweeping beam
- **Shooter Station** → shields its launch bay while constructing shooter ships
- **Reflector Gunship** → fires twin cannons while alternating a reflective shield

---

## 🧩 Levels

The current campaign contains **9 connected levels**, each with three authored waves:

- **Blue Frontier**
- **Emerald Crossing**
- **Shattered Belt**
- **Rose Siege**
- **Asteroid Wake**
- **Frozen Graveyard**
- **Crimson Entry**
- **Ion Storm**
- **Void Threshold**

Completed levels can be replayed through the Select Level menu without rewinding
campaign progress.

---

## ⚙️ Requirements

- C++23 compatible compiler
- Visual Studio 2026
- SFML 3.1.0 (64-bit)

📦 Setup guide:  
👉 [SFML Setup](libs/SFML/README.md)

---

## ▶️ Run

1. Open `Asteroid.slnx`
2. Build the project (Debug or Release)
3. Make sure SFML DLLs are available next to the executable
   > DLLs can be found in the SFML `bin` folder
5. Run the game

---

## 📁 Project Structure

```
src/        → game source code
assets/     → textures, sounds, fonts
design/     → visual direction and production specifications
tests/      → focused performance benchmarks
libs/       → external libraries (SFML)
build/      → compiled binaries (ignored)
```

---

## 💡 About the Project

This project was created as a **portfolio piece** to demonstrate:

- Object-oriented game architecture
- Input handling system (ActionMap + InputHandler)
- Entity-based design
- Real-time game loop and event processing
- Resource management (AssetStore)

---

## 📌 Tech Stack

- **C++23**
- **SFML 3.1.0**

---

## 🧑‍💻 Author

Demian Kozachuk
- 📧 Email: demianblogan@gmail.com

