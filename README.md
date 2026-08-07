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
3. Run `Asteroid.exe`

---

## 🕹 Controls

Default controls (all bindings can be changed in Options):

| Action        | Input              |
|--------------|-------------------|
| Move         | W A S D           |
| Aim          | Mouse             |
| Shoot        | Left Mouse Button |
| Pause        | ESC               |

---

## 🧠 Game Features

- 5 handcrafted levels
- Dynamic enemy waves system
- Score system with scaling difficulty
- Player physics (acceleration, damping, max speed)
- Screen wrapping (Asteroids-style world)
- Sound effects and background music
- HUD (score + lives)
- Skippable company splash screen
- Animated sci-fi main menu
- Pause menu with Resume and Back to Main Menu
- Persistent Graphics, Audio, and Controls settings
- Fullscreen, Windowed, and Borderless display modes
- Rebindable keyboard and mouse controls
- Bloom-highlighted UI, menu cursor, and gameplay crosshair

---

## 👾 Enemies

### 🪨 Meteors
- **Big** → slow, splits into medium (+20 pts)
- **Medium** → medium speed, splits into small (+60 pts)
- **Small** → fast (+100 pts)

### 🚀 Saucers
- **Kamikaze** → aggressively chases the player (+50 pts)
- **Shooter** → moves and shoots at the player (+200 pts)

---

## 🧩 Levels

The game contains **5 levels** with increasing difficulty:

- More enemies
- Faster spawn rates
- Mixed enemy types
- Combined wave mechanics

---

## ⚙️ Requirements

- C++23 compatible compiler
- Visual Studio 2026
- SFML 3.1.0 (64-bit)

📦 Setup guide:  
👉 [SFML Setup](libs/SFML/README.md)

---

## ▶️ Run

1. Open `Asteroid.sln`
2. Build the project (Debug or Release)
3. Make sure SFML DLLs are available next to the executable
   > DLLs can be found in the SFML `bin` folder
5. Run the game

---

## 📁 Project Structure

```
src/        → game source code
assets/     → textures, sounds, fonts
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

