<div align="center">

[![Windows Build](https://github.com/Detrend/nc/actions/workflows/build.yml/badge.svg)](https://github.com/Detrend/nc/actions/workflows/build.yml)

<div align="left">

## Project Nuclidean – Retro Doom-Style FPS
NUCLIDEAN is a fast-paced, retro-inspired FPS in the spirit of Doom, built from scratch in C++. What started as a university project is evolving into a full commercial release planned for Steam.

[Steam Page](https://store.steampowered.com/app/4526710/N_U_C_L_I_D_E_A_N/)

[Itch Io Page](https://ndt-games.itch.io/nuclidean)

---

## Made Without An Engine
NUCLIDEAN is developed without a third-party engine. Instead, we built a custom engine in C++ tailored specifically to the needs of our game.

This approach allows us to:
- Achieve high performance by leveraging the simplicity of the game world
- Implement efficient techniques like lightweight occlusion culling
- Maintain a fully deterministic game loop for easier testing and profiling

---

## Features
- Custom-built C++ engine (no Unity/Unreal/Godot)
- Non-euclidean levels with recursive portals
- Deterministic game loop for reproducibility, testing and recording player demos
- Retro-style rendering inspired by classic FPS games
- Custom physics and collision detection
- Custom renderer with fast occlusion culling that handles up to 30 shadow-casting lights and renders recursive portals
- Enemies that can pathfind and shoot at the player

Even though we programmed most of the game and engine by ourselves, we used the following libraries:
- SDL3 for OS-independent window handling
- ImGui for debug visualization
- NlohmannJson for JSON parsing
- GLM for 3D game math
- GLEW for OpenGL bindings

---

## Supported Systems
At the moment, the NUCLIDEAN can be played and built only on Windows. Linux support is planned in the future.

---

## Building

We hate C++ build systems just as the next guy and to avoid unnecessary complications we are building using Visual Studio only.
Transitioning to Premake/CMake is planned in the near future.

---

## Open Source
We plan to keep the source open (haven't decided on a licencing yet) so other game developers can peek under the hood and see how NUCLIDEAN works internally.

---
