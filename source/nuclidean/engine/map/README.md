## Map

### Map System / Sector System
The sector system is defined in the file [map_system.h](map_system.h).

Note that the sector system is not an engine module, but a set of systems that are owned and controlled from the game system.

Each level is composed of a set of "sectors". Each sector is a convex 2D polygon made of "walls". Each wall is made of 2 points and shares one point with the next wall of the sector and one point with a previous wall of the sector. A wall can be also shared between two sectors. This shared wall is called "portal". Portals can be both "normal" and "non-euclidean".
- Normal portal - just a wall shared between 2 sectors. The wall is not rendered and objects can walk through it to the other sector.
- Non-euclidean portal - is literally a portal and traversing it will seamlessly teleport an object to other part of the map. Non-euclidean portals are always paired, meaning that traversing through one side of the portal will teleport an object to other one. These portals are also rendered in a special way to make them look like they are normal portals and not an illusion.

Besides walls, each sector has a floor height and a ceiling height associated with it. Floors and ceilings of each sector are only horizontal and the walls can't be "tilted" - they are perpendicular to the floors/ceilings.

The sectors of a level are created once on the start of the level and during the runtime there are no new sectors created or old ones destroyed. Overall, sectors are quite static except a few exceptions:
- Floor height and ceiling height of a sector can change. This allows the level designer to create doors moving up or down, or elevators.
- Textures of walls of a sector can change. One usage of this are buttons on walls that change color when activated by the player.

Each sector has an "Sector ID" associated with it, which is it's unique integral identifier. The ID range starts at 0 and is continuous, meaning if there are sectors with IDs X and X+2 then there must also be an sector with ID X+1.

Identifying sectors with integral IDs instead of pointers has several benefits:
- Integral IDs, unlike pointers, remain the same after reloading a level. They can be therefore stored in a savefile.
- IDs are smaller than pointers *(can be 16bit or 32bit as there are not that many sectors)*. Therefore, they can be easily stored in framebuffer on a GPU.
- Accessing an invalid ID does not crash the game unlike accessing invalid pointer.
- An ID can be always converted to a pointer by offseting the address of the first sector by it.

Each wall of a sector is then identified by a "Wall ID". These are continuous and unique within each sector. The reasoning for using integral IDs is the same as for sector IDs.

Each wall of a sector then consists of several "segments". Segments are once again identified by integral IDs. Each segment has a start and end height and each segment can have a different texture.

### Map Dynamics
The runtime changes to sector system are handled by ["sector dynamics"](map_dynamics.h) subsystem, which is ran each frame and updates data of all sectors that should change.

It also works as a partial data driven "scripting" language. The map dynamics system allows the level designer to define a set of "activators", which when activated, change certain properties of sectors as floor height or ceiling height.

Activators are activated by "triggers" - events in the game. There are several types of triggers:
- Sector - gets triggered if there is an enemy or player within a certain sector.
- Wall - gets triggered if the player or enemy activates a button on a wall.
- Entity - gets triggered if an entity exists or does not exist *(for example, an enemy dies)*.

Each activator has a set of triggers associated with it. If the number of active triggers is greater than a certain designer-defined threshold then this activator becomes **active**.

Each sector can have 2 predefined dynamic states - ON and OFF, and can be associated with an activator. Each one of these states can have an arbitrary floor height and ceiling height. If an activator of a given sector becomes active then the sector starts transitioning into the ON state. If the activator becomes inactive then the sector starts transitioning back to the OFF state. This way, elevators or moving doors can be implemented.

Activators can also have arbitrary game logic attached to them via a list of `IActivatorHook`s. These hooks receive update when their parent `Activator`'s state changes and can perform various things like e.g. starting level transition, periodically damaging whatever entity is activating their `Activator` (e.g. by standing in a specific `Sector`) etc. . 

#### Sector Mapping
Allows efficient spacial queries for enities. Maintains a mapping `sector id` -> `list of entities` and `entity id` -> `list of sectors` which allow us to quickly query a list of sectors an entity is inside of, or a list of entities that are at least partially within a sector. The mapping has to change only if an entity moves or gets destroyed/created.

The mapping structure is maintained in the [sector_mapping.h](../entity/sector_mapping.h) file.

### Physics
There is an separate file [physics.h](physics.h) that contains implementation of the collision detections and game physics.

The physics are implemented around a "physical level" wrapper, which connects [entity system](), [map system](README.md) and [sector mapping]() together to have information about all physics related objects in the game.

The system implements a continuous collision detection that is performed by casting rays or cylinders agains physical objects in the level *(as entities or sectors)*. The `move_character` function also implements NPC-like movement which allows walking up/down a stairs.

#### Non-euclidean portals and physics
All physics related functions work with the portals by default. For example, casting a ray into a portal will make it traverse to the other side of the portal and collide with objects behind it.

The character movement function has to account for portals as well and reports a list of portals an object traversed through. After an object *(for example a player)* traverses through a portal, its velocity, position and direction have to be changed by a transformation matrix of the portal.

#### Acceleration Structures
To query a sectors on a certain position in the level a 2D grid structure is used. Each cell of the grid contains IDs of all sectors that are within the grid cell. The grid does have to be populated only once at the start of the level as the sectors are purely static and do not move.