## Engine Core
Before reading this, first see the [high-level engine documentation](../README.md).

The engine starts by a call to the function `init_engine_and_run_game` defined in [engine.cpp](engine.cpp) which receives a set of command line arguments. Potential command line arguments are:
- `-benchmark` - Runs benchmarks on profiling or release builds.
- `-unit_test` - Runs unit tests on non-deploy builds.
- `-test_filter=` - Allows running only tests with a certain name by filtering them using a regular expression.
- `-start_level [level_name]` - Starts the given level instantly after the engine intialization, skipping the menu screen.
- `-start_demo [demo_name]` - Starts a demo/replay with the given name. Exits the engine after the demo ends.
- `-fast_demo` - Plays the demo as quickly as possible.

The modules of the engine are intialized in the function `Engine::init` and the main loop takes place in `Engine::run`.

To add a new module, implement the `IEngineModule` interface from [`engine_module.h`](engine_module.h) and add a new item to the `EngineModuleId` enum in [`engine_module_types.h`](engine_module_types.h).

New module events can be defined in the [`module_event.h`](module_event.h).