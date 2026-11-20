## Engine Core
Before reading this, first see the [high-level engine documentation](../README.md).

The engine starts by a call to the function `init_engine_and_run_game` defined in [engine.cpp](engine.cpp) which receives a set of command line arguments. Potential command line arguments are:
- `-benchmark` - Runs benchmarks on profiling or release builds.
- `-unit_test` - Runs unit tests on non-deploy builds.
- `-test_filter=` - Allows running only tests with a certain name by filtering them using a regular expression.
- `-start_level [level_name]` - Starts the given level instantly after the engine intialization, skipping the menu screen.
- `-start_demo [demo_name]` - Starts a demo/replay with the given name. Exits the engine after the demo ends.
- `-fast_demo` - Plays the demo as quickly as possible.
- `-demo_playlist [entry ...]` - Plays the given demos one after another in an endless loop, without the menu or HUD. Each entry is `demo_name[:start_sec[:duration_sec]]` (the `.ncd` extension is optional). The part of the demo before `start_sec` is fast-forwarded on load. The demo switches after `duration_sec` seconds or when it ends, with a short fade out/in. Every loop starts each demo from its `start_sec` again. Can be combined with `-presentation`, in which case the slides stay unaffected by demo switches.
- `-demo_segment [sec]` - Default duration for playlist entries that do not specify their own. Without it such entries play until the demo ends.
- `-presentation [slide ...]` - Shows the given slide textures (switched by left/right arrows) with demos playing in the background.

Example: `NUCLIDEAN.exe -presentation slide_1 slide_2 -demo_playlist lvl1_run:15 lvl2_run:0:30 lvl3_run -demo_segment 20`

The modules of the engine are intialized in the function `Engine::init` and the main loop takes place in `Engine::run`.

To add a new module, implement the `IEngineModule` interface from [`engine_module.h`](engine_module.h) and add a new item to the `EngineModuleId` enum in [`engine_module_types.h`](engine_module_types.h).

New module events can be defined in the [`module_event.h`](module_event.h).