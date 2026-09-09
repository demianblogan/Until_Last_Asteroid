# Design patterns

How this codebase maps onto the common game-programming patterns — what each one
solves, how it's built here, and what it costs. Reference frame: Robert Nystrom's
*Game Programming Patterns*.

Some of these were designed in deliberately (the state stack, the command-style
input layer, the effect event queue); others emerged naturally (the asset facade,
the dirty flag). Both are called out.

- [Program-shaping patterns](#program-shaping-patterns)
- [Screens and flow](#screens-and-flow)
- [Entities and behaviour](#entities-and-behaviour)
- [Input and communication](#input-and-communication)
- [Resources and performance](#resources-and-performance)
- [Deliberately not used](#deliberately-not-used)
- [Summary](#summary)

---

## Program-shaping patterns

### Game Loop

**Problem.** A game can't block on input like a console program. It has to run
continuously — read input, advance the world, draw — at a rate independent of how
fast the machine is.

**Here.** [`Application::Run`](../src/app/Application.cpp):

```cpp
sf::Clock clock;
while (window.isOpen())
{
    const float frameTime = clock.restart().asSeconds();
    const float deltaTime = std::min(frameTime, MaxFrameTime);   // clamp

    HandleInput();
    Update(deltaTime, frameTime);
    Render();
}
```

Every piece of movement is multiplied by `deltaTime`, so the ship travels at the
same speed at 30 and at 144 FPS. `std::min(frameTime, MaxFrameTime)` is the safety
valve: if the game hangs for a second (alt-tab, a Windows dialog), a raw
`frameTime` of 1.0 would teleport everything a full second forward — through walls
and enemies. The clamp caps the simulated step so a stall slows the game instead
of breaking it.

**Trade-off.** This is a *variable* timestep. Simple, but the simulation is
slightly non-deterministic — the same inputs at different frame rates diverge by a
hair. A fixed timestep (accumulate real time, step the simulation in fixed 1/60 s
slices, decoupled from rendering) removes that; it matters for fighting games and
netcode, not for this one.

### Update Method

**Problem.** Dozens of objects — ship, enemies, bullets, particles, HUD — each
need to live through one frame in their own way.

**Here.** Everywhere. [`Entity::Update(float)`](../src/core/Entity.h) is pure
virtual and every enemy/projectile implements its own; so do `State::Update`,
`StateStack::Update`, and each `World` sub-system.

**Trade-off.** Iteration order matters — a bullet updated before an enemy registers
its hit this frame, after it next frame. And you **cannot mutate the collection**
(add/remove entities) while iterating it. That constraint is what drives the
deferred-change pattern below.

### Dependency injection via a context object

**Problem.** `OptionsState` needs the window, settings, audio, localization,
gamepad… `GameplayState` needs almost all of that plus the campaign save, records
and achievements. How do you hand those services out without making them global?

**Here.** One struct of references, passed to every screen's constructor —
[`StateContext`](../src/states/State.h):

```cpp
struct StateContext
{
    sf::RenderWindow& window;
    Assets& assets;
    SettingsManager& settings;
    CampaignSaveManager& campaignSave;
    RecordsManager& records;
    AchievementManager& achievements;
    LocalizationManager& localization;
    AudioManager& audio;
    // ...
};
```

**This project has no singletons** (`grep -rn "GetInstance\|s_instance" src/` is
empty). All ownership lives in `Application` and is handed down by reference. The
payoff: a class's dependencies are visible in its signature, services can be
swapped for tests, and there's no singleton-destruction-order crash on exit.

**Trade-off.** The context grows — ~14 fields now. Fine at this size (a reference
is 8 bytes); past ~40 fields it would be worth splitting.

---

## Screens and flow

### State — in two forms

The most important behavioural pattern in the game, implemented **two different
ways** — a good illustration that a pattern is not one fixed implementation.

**As objects (screens).** Each mode — splash, menu, gameplay, pause, results — is
a class with a shared interface. Switching mode swaps the object, instead of a
tangle of `if (mode == MENU)` branches.
[`State`](../src/states/State.h), 15 concrete screens in `src/states/`.

**As an enum FSM (the boss).** Cybermind has ~17 phases (arriving → outer ring
exposed → shield warning → shield → ring destroying → inner phase → … → core dying
→ victory). Seventeen classes sharing the same data would be more code than a
`switch`, so it's `enum class State { … }` + one field + `switch (state)` in
`Update()`. [`BossEncounter.h`](../src/gameplay/BossEncounter.h).

**Choosing between them:** lots of *dissimilar* behaviour, few states, long-lived →
classes. Many states that *share data* and differ in timing detail → enum +
switch. Same pattern, same point: an explicit state field instead of scattered
booleans.

### Pushdown Automaton (state stack)

**Problem.** Plain State can only *replace* a screen. Pause is not a replacement:
Esc in gameplay → pause menu appears *over* the frozen game → Resume → back exactly
where you were, combat state intact.

**Here.** [`StateStack`](../src/states/StateStack.h) — a `std::vector` of entries,
`PushState / PopState / ClearStates`, and an `IsTransparent()` flag so a screen can
let the one below it keep rendering (the frozen game behind the pause menu).

Two implementation details worth knowing:

**Deferred changes.**

```cpp
void PushState(StateID id) { pendingChanges.push_back({Action::Push, id}); }
void ApplyPendingChanges();   // the stack is only really mutated here, at frame end
```

`GameplayState::Update()` can decide "time for the results screen" and call
`RequestClear() + RequestPush(Results)` **from inside its own `Update`**. If the
stack mutated immediately, we'd destroy the object currently executing (`this`) —
a crash. Deferring to the frame boundary prevents it. This is the same rule as
"don't mutate a collection while iterating it", promoted to a pattern.

**State caching.** `EnableStateCaching(id)` keeps a popped screen alive and reuses
it on the next push, calling `OnReactivated()` to reset transients. A tiny object
pool — don't rebuild an expensive screen (achievement data, layout) every visit.

### Template Method

**Problem.** Nine menu screens do the same thing at the start and end of every
frame — tick the chrome (background + cursor + fade), check for a running
transition, draw the background. Only the middle differs.

**Here.** [`MenuState`](../src/states/MenuState.h):

```cpp
class MenuState : public State {
    void Update(float dt) final {        // final — a subclass cannot override
        chrome.Update(dt);
        if (finished_transition) fire_it();
        OnUpdate(dt);                     // the hole
    }
    virtual void OnUpdate(float dt) {}    // subclass fills only this
    virtual void OnRender() = 0;
};
```

The base owns the skeleton; the subclass fills the holes. Note the inversion: the
**base calls the subclass**, not the other way round. A screen can't forget to
update the chrome or get the step order wrong, because it never writes those steps.

**Trade-off.** A rigid skeleton. `PauseState` didn't fit (it draws a blurred
snapshot, not the parallax background) and stayed on plain `State`. The header
comment lists who opts out and why — the pattern is a tool, not a law.

---

## Entities and behaviour

### Subclass Sandbox

**Problem.** Nine enemy types. Each needs to fire from its own emitter points, fly
to an entry point before starting its pattern, take damage, drop rewards. If every
enemy reaches into `World` and `Assets` directly, you get nine copies of the same
code and nine ways to get it subtly wrong.

**Here.** [`Enemy`](../src/entities/enemies/Enemy.h) hands subclasses a **closed
set of safe operations** — a sandbox:

```cpp
protected:
    sf::Vector2f GetWeaponEmitterPosition(std::size_t index) const;  // the formula every shooter repeated
    void UpdateApproach(float dt);          // "fly to the entry point"
    float GetMovementSpeed() const;
    // health / reward machinery is private to the base
```

[`KamikazeSaucer`](../src/entities/enemies/KamikazeSaucer.h) is then literally
`void Update(float dt)` ("chase the player, spinning") plus one float. The base's
own `Update` only knows "fly in a straight line"; everything else the subclass
builds from the supplied blocks. This is what keeps nine enemies predictable and
editable.

Usually pairs with Template Method: Template Method decides *when* the subclass is
called, Subclass Sandbox decides *what it may use* while it runs.

### Type Object + data-driven design

Probably the strongest architectural decision in the project.

**Problem.** Balance is hundreds of numbers — kamikaze health 20, speed 500, score
200, shooter fire interval 1.0 s, turret beam damage 20… If they're compiled into
`.cpp`, every tweak is a full rebuild and a new build for players.

**Here.** [`GameplayData`](../src/gameplay/GameplayData.h) loads and **validates**
at startup:

```
assets/data/gameplay/
  enemies.json   -> an EnemyConfig per type
  player.json    -> PlayerConfig
  levels.json    -> 10x LevelConfig (background, brightness, target accuracy, waves)
  boss.json  weapons.json  pickups.json  effects.json
```

Instead of "a `Kamikaze` class with its numbers baked in", there's one code class
plus an `EnemyConfig` object that *is* the type. `Enemy`'s constructor takes
`const GameplayData::EnemyConfig&` and copies the numbers in. The class is
behaviour; the config is the species' stat block.

What it buys: retune balance in a text file with no rebuild; "double-HP kamikaze"
for Horde is data, not a new class; all validation is in one place, throwing on a
malformed JSON at load rather than crashing an hour into a fight.

**Trade-off.** A layer of indirection — to understand the turret you read both
`LaserTurret.cpp` and `enemies.json` — plus the load/validate code itself. Worth it
when the numbers are many and change often, which is exactly this case.

### Strategy — in spirit

- **Enemies.** Each `Enemy` subclass is effectively a movement/attack strategy,
  picked at spawn by `switch (spawn.kind)`. Classic Strategy swaps the strategy on
  a *live* object; here it's fixed at construction — closer to "polymorphism +
  factory" — but the thinking is the same.
- **[`VibrationProfiles`](../src/gameplay/VibrationProfiles.h)** is cleaner: a
  generic `GamepadHaptics` primitive ("spin the motors at X, Y for T seconds")
  plus a separate table of named presets (`Light`, `MenuNavigation`, `Collision`,
  `Death`). The motor knows nothing about the game; the game picks a preset. That's
  mechanism/policy separation — a Strategy relative where the "strategy" is a
  three-field struct.

Takeaway: not every Strategy needs a class hierarchy. Sometimes it's an `enum`, a
data `struct`, or a `std::function`.

---

## Input and communication

### Command

**Problem.** "Fire" is the left mouse button. Or the right trigger. Or button 5 on
a stick with no trigger axis. Or whatever the player rebinds it to. The firing
code must not know about buttons; the button code must not know about firing.

**Here** — small but clean:

- [`InputBinding`](../src/input/InputBinding.h) — one physical binding:
  `std::variant<Key, MouseButton>` + `TriggerType { OnPress, OnRelease, WhileHeld }`
- [`ActionMap<Action>`](../src/input/ActionMap.h) — the table `Action ->
  vector<InputBinding>` (data only)
- [`InputHandler<Action>`](../src/input/InputHandler.h) —
  `Subscribe(Action, callback)`; `HandleEvent` / `Update` walk the table and invoke
  the callbacks of triggered actions

The point of Command is to **reify the intent** ("fire") as something detached from
how it was invoked. That gives key rebinding (change the `ActionMap`, don't touch
game code — the Controls settings menu) and unified keyboard/gamepad handling (the
gamepad is just another source filling the same `Action`s) for free.

**C++ note.** GoF draws Command as a class with `execute()`. Here a command is a
`std::function<void()>`. Undo/redo needs the full object (store and reverse it);
input just needs the lambda.

### Observer

**Problem.** When the player clears level 5, several things must happen — unlock an
achievement, show a toast, write a record, play a sound. `GameplaySession` must not
know about the achievement system, the UI, or records.

**Here.** `InputHandler::Subscribe`; `AchievementManager` as the hub that different
systems report progress to, with `AchievementToast` showing the result.

**Trade-off.** Observer makes control flow *implicit* — reading `GameplaySession`
you don't see that `CompleteLevel()` fans out across half the game. That's the cost
of decoupling. When the subscriptions get numerous and interact, move to the next
pattern.

### Event Queue

**Problem.** While `World::Update()` resolves combat, dozens of visual events fire
— hits, explosions, muzzle flashes, missile smoke. Rendering and simulation are
separate subsystems; calling the renderer from the middle of collision resolution
welds them together.

**Here.** [`WorldEffectEventQueue`](../src/core/world/WorldEffectEventQueue.h) +
[`EffectEvent`](../src/rendering/EffectEvent.h):

```cpp
// during Update, in World:
effectQueue.Add({ EffectEventType::AsteroidExplosion, position, ... });
// once per frame, in the renderer:
for (auto& e : world.GetEffectEvents()) effects.Spawn(e);
world.ClearEffectEvents();
```

The event vocabulary (`enum EffectEventType`) lives *next to the renderer* — its
only consumer. `World` only *produces* events, never reads them, so `World` can be
understood without knowing anything about particles.

**Event Queue vs Observer.** Observer is synchronous (notify → subscriber runs now,
in the same stack). Event Queue is asynchronous (enqueue → drained later, in
another place) and it **batches** — 50 hits in a frame are handled in one pass at a
predictable moment. `StateStack::pendingChanges` is the same pattern applied to
structural changes instead of effects.

---

## Resources and performance

### Facade

**Problem.** Loading resources means textures, fonts, sounds, music, shaders,
cursors and JSON data — each with its own load path and storage. The rest of the
game shouldn't know about all seven subsystems.

**Here.** [`Assets`](../src/assets/Assets.h) fronts four `AssetCache`s + a shader
map + a cursor map + `GameplayData`, exposing `assets.Textures().Get(id)` and
`assets.GetShader(id)`. `MenuChrome` fronts "background + cursor + fade". `World`
fronts its 7–8 systems.

**Trade-off.** A facade tends to swell into a god-object. `World` has 38 public
methods; `GameplayState` is large too. The tell that a facade has gone wrong: it
stops *delegating* to subsystems and starts *containing* the logic. The fix is to
extract systems — which is what the v2.0 refactor did.

### Factory Method

**Problem.** `StateStack` must create any screen from a `StateID` without depending
on 15 screen headers or knowing each constructor's arguments.

**Here.** [`StateStack::RegisterState`](../src/states/StateStack.h):

```cpp
template <typename StateType, typename... Arguments>
void RegisterState(StateID id, Arguments... args) {
    factories[id] = [/* captured args */] {
        return std::make_unique<StateType>(*this, context, args...);
    };
}
```

`factories` is `unordered_map<StateID, std::function<unique_ptr<State>()>>`;
`CreateState(id)` calls `factories[id]()`. A simpler second example:
`GameplayState::SpawnConfiguredEnemy` with `switch (kind)` — a switch-factory, fine
when the type set is small and fixed.

### Flyweight / resource cache

**Problem.** 40 small asteroids on screen. The asteroid texture is 200 KB. Loading
it 40 times wastes 8 MB and does 40 disk reads.

**Here.** [`AssetCache<Resource, IdEnum>`](../src/assets/AssetCache.h) loads a
resource once (`LoadFromFile(id, path)`) and returns a `Resource&` on `Get(id)`.
Every `Entity` takes `sf::Texture&` — a reference to the one shared object, not a
copy. 40 asteroids = 1 texture + 40 light `sf::Sprite`s (position/rotation/scale is
the unique per-instance state).

Flyweight formally splits state into *intrinsic* (shared, immutable — the texture
pixels) and *extrinsic* (unique — where to draw). In practice that's the same as
"load once, share by reference".

### Dirty Flag

**Problem.** The UI glow (bloom) is expensive — render to texture, bright-pass
shader, several blur passes. But a button's content changes rarely (on hover),
while we draw 60 times a second.

**Here.** [`NeonGlow`](../src/rendering/NeonGlow.h):

```cpp
bool needToRebuild = true;
void Invalidate() noexcept { needToRebuild = true; }
void DrawBloom(...) {
    if (needToRebuild) { Rebuild(...); needToRebuild = false; }
    // ... draw from the cache
}
```

`Invalidate()` calls sit wherever the glowing content changes — a new selected
button, rebuilt settings rows, a resize. Between them, 60 frames draw the cached
texture for free.

**Trade-off — the classic trap.** Forget one `Invalidate()` where the content
changed and you get *stale* glow, and a long hunt. Rule: the code that mutates the
data raises the flag, at the moment of mutation.

### Double Buffer

**Problem (graphics).** Drawing straight to the screen one object at a time shows a
half-drawn frame (tearing, flicker). Draw the whole frame into a hidden buffer,
then present it at once.

**Here, on two levels:**

1. **Explicitly, in post-processing.** `GameplayState` renders the scene to an
   *offscreen* texture; `GameplayPostProcessor` runs it through a chain
   (bright-pass → Gaussian blur → composite with grading/vignette/distortion); only
   the result reaches the screen. The intermediate textures are buffers.
2. **As a principle**, in `StateStack::pendingChanges` and `WorldEffectEventQueue`
   — "collect all changes in a side buffer, apply them at the frame boundary" is
   Double Buffer applied to data structures instead of pixels. The deferred-changes
   pattern, the event queue, and double-buffered rendering are one idea in three
   forms.

---

## Deliberately not used

Knowing when *not* to reach for a pattern is part of the skill. Absent on purpose:

- **Singleton.** DI via `StateContext` instead (see above). Convenient right up
  until you need tests or a clean shutdown order.
- **Full ECS.** Classic OO inheritance (`Entity -> Enemy -> KamikazeSaucer`). ECS
  earns its keep with thousands of entities or a combinatorial explosion of
  behaviour mixes. ~50 entities on screen and a fixed set of 9 enemy types — OO is
  simpler and more readable here.
- **A full OO State machine for the boss.** 17 state classes sharing one data blob
  is more code than a `switch`. Enum FSM chosen instead.
- **A strategy hierarchy for game modes.** The v2.0 refactor considered extracting
  Campaign/Horde/Run into an `IGameMode`; rejected because the modes reach into
  ~15 `GameplayState` members and there's no clean seam — the abstraction would
  have leaked. Kept `enum class GameMode` + branching. An honest trade:
  `GameplayState` stays large but doesn't gain a fake architecture.

---

## Summary

| Pattern | Where | Solves |
|---|---|---|
| Game Loop | `Application::Run` | continuous execution, frame-rate independence |
| Update Method | `Entity::Update`, every system | each object lives its own frame |
| Dependency injection | `StateContext` | hand out services without globals |
| State (objects) | `State` + 15 screens | game modes without an `if` maze |
| State (enum FSM) | `BossEncounter::State` | 17 boss phases sharing data |
| Pushdown Automaton | `StateStack` | pause over gameplay, return to it |
| Deferred changes / Double Buffer | `StateStack::pendingChanges` | safely mutate the stack from inside `Update` |
| Template Method | `MenuState` | shared skeleton for 9 menu screens |
| Subclass Sandbox | `Enemy` | 9 enemies on one safe toolkit |
| Type Object + data-driven | `GameplayData` + JSON | balance without a rebuild |
| Command | `ActionMap` / `InputBinding` / `InputHandler` | key rebinding, one input path for keyboard and pad |
| Observer | `InputHandler::Subscribe`, `AchievementManager` | decouple "event -> reactions" |
| Event Queue | `WorldEffectEventQueue` | decouple simulation from rendering, batch |
| Facade | `Assets`, `World`, `MenuChrome` | a simple front over many subsystems |
| Factory Method | `StateStack::RegisterState` | build a screen from a `StateID` |
| Flyweight / resource cache | `AssetCache` | one texture for 40 asteroids |
| Dirty Flag | `NeonGlow` | don't rebuild bloom every frame |
| Double Buffer | post-processing, offscreen targets | a whole frame at once, an effect chain |
