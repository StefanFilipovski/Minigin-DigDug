# Dig Dug — 2DAE Programming 4 Exam

A Dig Dug clone built on a small custom game engine (**Minigin**) using **SDL3**.

## Source control

https://github.com/StefanFilipovski/Minigin-DigDug

The grading script clones this repository with `git clone --recurse-submodules`.

## Building

This is a standard CMake project. All third-party dependencies are fetched
automatically with `FetchContent`, so no manual setup is required.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The `DigDug` executable is produced together with its `Data` folder and the required
SDL runtime DLLs (copied next to the executable as a post-build step).

- **Targets:** `Minigin` (static engine library), `DigDug` (game executable), `imgui`.
- **Dependencies (auto-fetched):** SDL3, SDL3_ttf, SDL3_mixer, glm, Dear ImGui.
- **C++ standard:** C++20.
- Visual Leak Detector (VLD) is used automatically when found, otherwise it is skipped.

## Controls

| Action               | Player 1 (keyboard) | Player 2 / Versus (controller) |
|----------------------|---------------------|--------------------------------|
| Move                 | WASD                | D-Pad                          |
| Pump / Action        | Space               | A                              |
| Fygar fire (Versus)  | —                   | A                              |
| Fygar ghost (Versus) | —                   | B                              |

In single player, Player 1 can play entirely with the keyboard **or** entirely with
the first controller (D-Pad + A), including the menus and high-score entry.
In co-op and versus, the first controller belongs to Player 2 / the Fygar;
Player 1 (Dig Dug) plays on the keyboard or on a **second** controller, so both
two-player modes work with one keyboard + one gamepad or with two gamepads.

Debug keys: `F1` skip level, `F2` mute/unmute, `F3` test sound.

### Game modes
- **Single Player** — classic Dig Dug; scores are saved to the high-score table.
- **Co-op** — two Dig Dug players sharing one life pool; any death sends both back to spawn.
- **Versus** — one player is Dig Dug, the other controls a Fygar that breathes fire and can
  phase through dirt in ghost form (on a cooldown). First to run the other out of lives wins.

## Engine overview & design choices

**Minigin** is a lightweight, component-based engine. The main patterns used:

- **Game object / component model** — `GameObject`s own `Component`s (transform, render,
  text, sprite animator, and game-specific components). Update/render are propagated through
  the scene graph.
- **Command pattern** for input — `InputManager` maps keyboard scancodes and controller
  buttons to `Command` objects, so the same actions can be rebound per game mode. Cleared
  commands are kept alive in a graveyard for one frame so a command can safely rebind the
  input map while it is executing.
- **Observer pattern** — `Subject`/observers drive the HUD (score and lives displays update
  in response to game events) without coupling gameplay code to UI.
- **Service locator + event queue** for audio — gameplay code talks to an `ISoundService`
  interface. `SDLSoundService` wraps SDL3_mixer and processes sound requests on a worker
  thread through a queue (the engine's threading requirement). All audio is predecoded and
  cached; the looping music track self-heals via a stopped-callback and a watchdog, and
  muting silences the mixer gain so playback state survives mute/unmute. `NullSoundService`
  provides a silent fallback when no audio device is available.
- **Flyweight** — sprite animation data (frames, fps, loop flags) is built once per entity
  type as an immutable shared `AnimationSet`; each animator instance only stores its own
  playback state.
- **Type object** — enemy types (textures, animations, abilities, fire color key) are rows
  in a data table (`EnemyTypeInfo`) instead of code branches, so adding an enemy type is a
  data change.
- **State pattern** — game flow (`MenuState`, `PlayingState`, `GameOverState`,
  `HighScoreState`) is managed by `GameStateManager`; enemies use their own state machine
  (`EnemyStates`) for normal/ghost/inflating/fire-breathing behaviour.
- **Data-driven levels** — levels (grid, spawns, rocks) are loaded from JSON in `Data/Levels`
  using nlohmann/json.
- **Reproducible builds** — every dependency is pulled with `FetchContent` at configure time,
  so the project builds from a clean checkout with no external library setup.

Singletons are explicitly torn down at shutdown so Visual Leak Detector does not report their
owned resources as false-positive leaks.
