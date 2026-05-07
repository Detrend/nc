## Nuclidean Codebase
### Grid
Defined in [`grid.h`](grid.h), grid is a spatial acceleration structure used for querying sectors in the level by theird position. It contains several functions for both point and area query.

### CVars
Defined in [`cvars.h`](cvars.h), cvars are a configuration variables that can be tweaked on the runtime to change the behaviour of the game. Some of these have a debugging character *(for example time speed)*. To open up a cvar configuarion menu ingame, press '`' key *(under ESC)*. Cvars are baked in during the deploy builds and cannot be changed by the user.

### Profiling
A support for simple in-game profiler. A profiler window can be accessed from the menu window by pressing the '`' key *(under ESC)*.

To profile a block of code, use the `NC_SCOPE_PROFILER(SectionName)` macro.

### Token
A static string implementation in the file [`token.h`](token.h) that supports fixed number of characters.

### Unit Test
Simple unit-test framework for defining tests. Tests can be on the startup by running the game with `-unit_test` cmd argument.