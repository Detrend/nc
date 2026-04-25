## Map

### Map System / Sector System
Each level is composed of a set of "sectors". Each sector is a convex 2D polygon made of "walls". Each wall is made of 2 points and shares one point with the next wall of the sector and one point with a previous wall of the sector. A wall can be also shared between two sectors. This shared wall is called "portal". Portals can be both "normal" and "non-euclidean".
- Normal portal - just a wall shared between 2 sectors. The wall is not rendered and objects can walk through it to the other sector.
- Non-euclidean portal - is literally a portal and traversing it will seamlessly teleport an object to other part of the map. Non-euclidean portals are always paired, meaning that traversing through one side of the portal will teleport an object to other one. These portals are also rendered in a special way to make them look like they are normal portals and not an illusion.

Besides walls, each sector has a floor height and a ceiling height associated with it. Floors and ceilings of each sector are only horizontal and the walls can't be "tilted" - they are perpendicular to the floors/ceilings.

The sectors of a level are created once on the start of the level and during the runtime there are no new sectors created or old ones destroyed. Overall, sectors are quite static except a few exceptions:
- Floor height and ceiling height of a sector can change. This allows the level designer to create doors moving up or down, or elevators.
- Textures of walls of a sector can change. One usage of this are buttons on walls that change color when activated by the player.

The runtime changes to sector system are handled by ["sector dynamics"](map_dynamics.h) subsystem, which is ran each frame and updates data of all sectors that should change.

Each sector has an "Sector ID" associated with it, which is it's unique integral identifier. The ID range starts at 0 and is continuous, meaning if there are sectors with IDs X and X+2 then there must also be an sector with ID X+1.

Identifying sectors with integral IDs instead of pointers has several benefits:
- Integral IDs, unlike pointers, remain the same after reloading a level. They can be therefore stored in a savefile.
- IDs are smaller than pointers *(can be 16bit or 32bit as there are not that many sectors)*. Therefore, they can be easily stored in framebuffer on a GPU.
- Accessing an invalid ID does not crash the game unlike accessing invalid pointer.
- An ID can be always converted to a pointer by offseting the address of the first sector by it.

Each wall of a sector is then identified by a "Wall ID". These are continuous and unique within each sector. The reasoning for using integral IDs is the same as for sector IDs.

Each wall of a sector then consists of several "segments". Segments are once again identified by integral IDs. Each segment has a start and end height and each segment can have a different texture.

#### Sector Mapping
Allows efficient spacial queries for enities. Maintains a mapping `sector id` -> `list of entities` and `entity id` -> `list of sectors` which allow us to quickly query a list of sectors an entity is inside of, or a list of entities that are at least partially within a sector. The mapping has to change only if an entity moves or gets destroyed/created.

### Physics