# Nucledian Level Editor

This is not the game itself, just the level editor.

To run, just open this in Godot 4.5.2 as one would with any normal game project.   


## General overview

The game Nucledian is a 2.5D, Doom-inspired boomer shooter. It's world is made of convex polygons - `Sector`s - laid out on a 2D plain. There can't be 2 sectors on top of each other, however a sense of verticality can somewhat be achieved by assigning different floor/ceiling heights to sectors. Also our non-euclidean portals can be used to emulate some kinds of sector-on-top-of-sector placements.   
The game doesn't support a scripting language - all game logic is written in C++ along with the game engine. Thus, all that is done in the Editor is laying out sectors, placing entities in them and assigning simple rules (`Triggers` - `Activators`) for level interactivity.   

The Level editor exports the level as a single JSON file, which then gets loaded by the game.  

---------------------------------
## How to use

For reference, look at `levels/testLevel1.tscn`.   


### `Level`
Root of a level must be a node of type `Level`.  
To export the level, set appropriate output path and other parameters and press the button `Export` in the node's inspector panel.   
- parameter `export_scale`... `x`,`y` components memberwise multiply the positions of sector points, `z` component multiplies the `floor` and `ceiling` heights of sectors
- `PlayerPosition` - place a single instance of this prefab somewhere among the `Level`'s descendants and the level will be shifted so that this prefab's position is the exported level's origin
- button `Snap` - snap all points to position of their nearest neighbour from different segment (as long as the neighbor is at most `max_snapping_distance` far away). _(It seems better practice to use Godot's builtin grid-snap (`Ctrl+G`) and thus not need this, but still it's sometimes nice to have.)_


---

### `Sector`
- Based on Godot's builtin `Polygon2D` node (has a nice editor).
- Create a new simple triangle sector by holding `Ctrl+Shift` while left-mouse-clicking on empty place in the scene.
	- If a single, non-`Sector` Node is selected, the new sector is assigned as its child, with default floor/height 
	- Otherwise, searches for the nearest existing sector point - the new sector will be created under the same parent and with the same floor/ceiling heights etc. as the found sector
- The editor does NOT enforce convexity in any way, user is 100% responsible for that.
- Editor makes automatically sure that the points are ordered clockwise and that there are not multiple points at the exact same position.
- It seems as a really good idea to enable Godot's grid-snap while configuring the sector points.
- If multiple sectors share a point - hold `Q` while moving that point to move the same point of the other sectors as well.
- `Hide` a sector to exclude it from level export
- Placing portals... should be intuitive, only one portal per sector is currently permitted. _One-sided portals are available for experimentation, but they assert-crash the game._
- `floor`, `ceiling`... visualized by sector's color 
	- see related properties in `config.tres` for customization
	- switch between `floor`/`ceiling` visualization via `Level`'s `coloring_mode` property
- Advanced editing:
	- Extrude... hold `X` while adding a point to the polygon - instead of adding a point, a new sector will be created by extruding from the sector's wall
	- Loop-cut... hold `C` while adding two points to the polygon - instead of adding two points, the sector will be cut in half along the two points
	- _Both of these actions are available for `TriangulatedMultisector` as well_


---

### `TriangulatedMultisector`
- Smarter editable polygon that can be arbitrarily non-convex. It automatically triangulates its area with sectors
- Add ordinary `Sector`s inside the shape as its children - they will act as "holes" in the triangulated polygon
- Create a new multisector by holding `Ctrl+Shift+R` while left-mouse-clicking on empty place in the scene
- Button "Bake" - copy the sectors that triangulate this multisector to a new blank parent, so that they can be manually fine-tuned  


---

### `Trigger`s - `Activator`s
- tool to achieve basic reactivity in the level (e.g. opening doors)
- `Activator`...
	- named object, holds a counter which gets in(/de)cremented by `Trigger`s
	- field `threshold`... if the counter reaches this value, the `Activator` is considered __activated__
	- a `Sector` can declare an `AltConfig` - alternative floor/ceiling heights, linked to an `Activator` - if the activator is active, the level will shift to the alternative heights
		- can be used e.g. to create doors (default floor-ceiling=(0.0, 0.01), alt=(0.0,2.0)), vertical moving platforms etc.
	- `ActivatorHook`... an object attached as a child node to an `Activator`, performs some special gameplay logic (e.g. level transition, kill-floor, jump pad)
		- Can be found in `scripts/reactivity/hooks`
- `Trigger`...
	- attached to an entity (triggered on life/death), sector (triggered on touching the floor) or a specific sector wall (triggered by clicking the wall with key `F`)
	- linked to an `Activator` - while triggered, increments the activator value  

---

### `Things`

Objects that can be placed in the game world. Most of them are static, some of them can move through the level during gameplay (e.g. enemies, the player).   
All existing entities have a prefab representing them (you technically don't need to use the prefab, but you always should, because it displays a nice icon at the thing's location). You can find those in the folder `prefabs` - subfolders:
 - `Entities`... enemies and the player
 - `Lights`... Point light, directional light, ambient light
 - `PickUps`... ammo, medkits etc.
 - `Props`... cosmetic bilboards to make the world look richer, do not interact with gameplay   
Just place an instance of the prefab into the game world, inside an area occupied by some `Sector`. Those that aren't inside any `Sector` will get excluded from export.   
To adjust vertical placement of a `Thing`, use the properties `placement_mode` (placement relative to sector floor/ceiling/in absolute numbers) and `height_offset` (gets added to floor_height / subtracted from ceiling_height / used as absolute height).
For `Entities` (most importantly the `Player`), use their world rotation to adjust which direction they are looking at.  


---

### Texturing
`Sector`'s field `sector_material` - assign one of the prepared materials in the `materials` folder.


---------------------------------

## Cheat Sheet
- `Alt + P`, `Alt + L`... increment/decrement selected sector's floor/ceiling (depending on current `coloring_mode`) height, useful when creating stairs
- `Ctrl + Shift + LMB`... create new `Sector`
- `Ctrl + Shift + R + LMB`... create new `TriangulatedMultisector`
- `Alt + T`... triangulate current multisector (only when a `TriangulatedMultisector` or its child is selected)
- `Q`... multi point movement (hold while moving a (multi)sector point) 
- `C`... loop cut (hold while adding 2 points to a (multi)sector)
- `X`... extrude (hold while adding a point to a (multi)sector)
