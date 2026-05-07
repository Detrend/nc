## Engine

### Engine Architecture
The core of the engine is defined in the directory `core/` and the most important file is [`engine.h`](core/engine.h) where the most necessary infrastructure is implemented.

However, the Nuclidean engine is decomposed into "modules" that are split into several other files and directories. Each module is responsible for a set of gameplay or engine systems.

#### Modules
List of modules and their responsibilities.
- [**Graphics module**](graphics/graphics_system.h) for rendering and handling OS window.
- [**Input system**](input/input_system.h) for processing user inputs and filtering them.
- [**Game system**](game/game_system.h) for handling game update and logic.
- [**Sound system**](sound/sound_system.h) for playing and handling sounds and music.
- [**UI system**](ui/user_interface_system.h) for handling and rendering of UI.

The engine can communicate with the modules both directly *(querying a pointer to a module and calling appropriate functions)* and indirectly - sending a ["module message"](core/module_event.h). Each module can override a function `on_event` which receives each event sent by an engine. The module can then react appropriately to the given event.

Examples of some module messages:
- Post init - sent to all modules after full initialization of the engine, but before running the first update.
- Frame start - sent as the first event of a new frame.
- Cleanup - sent at the end of a frame. Systems have opportunity to cleanup data that will not be needed in the next frame.
- Game update - game system specific, the game updates.
- Menu opened/closed - menu state changed.
- Level ended - the level ended.
- Before/after map rebuild - the map of a level changed/is about to change and systems have an opportunity to react to it.

Overall each module can send any messages to the engine and receive messages from all other modules except when the message explicitly bans a module from receiving it.
The order in which the modules receive a message is the same as their module ID, which is the same as their initialization order. However, the modules should not rely on this fact and if there is a certain message that has to be received by module A before some other module B than it is a good practice to split it into 2 distinct messages, each one for A and other one for B and then send these messages in correct order.

Initialization of the modules happens in the same order as their module ID and the deinitialization in the reverse order. If a module fails to initialize then the engine reports a failure and exits.

#### Demos and determinism
The game system is deterministic, meaning that if ran multiple times with the same inputs the outcome will be the same. There is an important caveat - the determinism only works if the FPS is the same during both playthroughs.

In practice, this means that we can record a frametime and list of pressed/released inputs for each frame of the game since a level starts and restarting this level and replaying the inputs *(together with the same update deltas)* will lead to the same outcome.

We use this capability in 2 ways:
- Debugging and profiling - the same level can be ran multiple times, helping us recreate hard to reproduce bugs or profile a performance of one playthrough multiple times.
- Design gimmick - a replay of player's actions can run on the background after finishing a level or in the background of the main menu.

#### Game state

The engine has a "game state" enum which determines the current state. The possible game states are:
- Menu - the game is in the main menu from where the user can start a new game or load old one. This is the initial state after the game starts. Randomly chosen demo of previous playthrough plays in the background.
- Game - the player is controlling the game.
- Transition - player finished a level and is in a transition screen. The replay of the playthrough is playing in the background, but is not controlled by the user.
- Debug demo - a demo of a certain level is played in the background. The engine can get into this state only if run with a specific command line arguments.

A table with detailed description what is happening in each game state follows.

| State      | Level loaded | User in control of game | Menu displayed | Transition screen displayed |
|:----------:|:------------:|:-----------------------:|:--------------:|:---------------------------:|
| Menu       | ✅           |                         | ✅            |                             |
| Game       | ✅           |✅                       |               |                             |
| Transition | ✅           |                         |                | ✅                         |
| Debug      | ✅           |                         |                |                            |


### Sector System
This is a set of systems that handle the environment of levels. For more info see [map system docs](map/README.md).

### Entity System
Entities are dynamic objects in the level like player, enemy or a light. A rule of thumb is that if an object is in the game and it is not a sector then it is probably an entity.

Whereas sectors are almost static, the entities can be dynamically created/destroyed during the gameplay and their data can be mutated.

Lifetime of entities is bound to the entity system and they never outlive the lifetime of their level.

For more info about entity system see [entity system docs](entity/README.md).

### Graphics
Graphics system is responsible for querying the visible parts of the level and then rendering them - both entities and sectors. It also handles rendering of non-euclidean portals.

### Game System
Updates the game and entities within it each frame.
