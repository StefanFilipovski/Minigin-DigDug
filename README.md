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

Debug keys: `F1` skip level, `F2` mute, `F3` test sound.

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
- **Service locator** for audio — gameplay code talks to an `ISoundService` interface.
  `SDLSoundService` wraps SDL3_mixer and processes sound requests on a worker thread (music
  is streamed and looped; effects are cached); `NullSoundService` provides a silent fallback
  and is swapped in for muting.
- **State pattern** — game flow (`MenuState`, `PlayingState`, `GameOverState`,
  `HighScoreState`) is managed by `GameStateManager`; enemies use their own state machine
  (`EnemyStates`) for normal/ghost/inflating/fire-breathing behaviour.
- **Data-driven levels** — levels (grid, spawns, rocks) are loaded from JSON in `Data/Levels`
  using nlohmann/json.
- **Reproducible builds** — every dependency is pulled with `FetchContent` at configure time,
  so the project builds from a clean checkout with no external library setup.

Singletons are explicitly torn down at shutdown so Visual Leak Detector does not report their
owned resources as false-positive leaks.
