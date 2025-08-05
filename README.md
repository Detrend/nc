## Quick and dirty level editor

> This is in no way intended to be used long-term, it's just a quickly put-together, unmaintainable piece of junk that will hopefully let us experiment with level design without needing to wait for the real level editor to be finished.

To run, just open this in Godot 4.4.1 as one would with any normal game project.   

To try out the exported level, use game branch `qd-level-import` and make sure `JSON_LEVEL_PATH` at the top of `thing_system.cpp` is set to the path to your exported level.


---------------------------------
### How to use

For reference, look at `levels/testLevel1.tscn`.   

#### `Level`
Root of a level must be a node of type `Level`.  
To export the level, set appropriate output path and other parameters and press the button `Export` in the node's inspector panel.   
- parameter `export_scale`... `x`,`y` components memberwise multiply the positions of sector points, `z` component multiplies the `floor` and `ceiling` heights of sectors
- `PlayerPosition` - place a single instance of this prefab somewhere among the `Level`'s descendants and the level will be shifted so that this prefab's position is the exported level's origin
- button `Snap` - snap all points to position of their nearest neighbour from different segment (as long as the neighbor is at most `max_snapping_distance` far away). _(It seems better practice to use Godot's builting grid-snap (`Ctrl+G`) and thus not need this, but still it's sometimes nice to have.)_

#### `Sector`
- Based on Godot's builtin `Polygon2D` node (has a nice editor).
- Create a new simple triangle sector by holding `Ctrl+Shift` while left-mouse-clicking on empty place in the scene.
	- If a single, non-Sector Node is selected, the new sector is assigned as its child, with default floor/height 
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

#### `TriangulatedMultisector`
- Smarter editable polygon that can be arbitrarily non-convex. It automatically triangulates its area with sectors
- Add ordinary `Sector`s inside the shape as its children - they will act as "holes" in the triangulated polygon
- Create a new multisector by holding `Ctrl+Shift+R` while left-mouse-clicking on empty place in the scene
- Button "Bake" - copy the sectors that triangulate this multisector to a new blank parent, so that they can be manually fine-tuned 


---------------------------------

#### Cheat Sheet
- `Alt + P`, `Alt + L`... increment/decrement selected sector's floor/ceiling (depending on current `coloring_mode`) height, useful when creating stairs
- `Ctrl + Shift + LMB`... create new `Sector`
- `Ctrl + Shift + R + LMB`... create new `TriangulatedMultisector`
- `Alt + T`... triangulate current multisector (only when a `TriangulatedMultisector` or its child is selected)
- `Q`... multi point movement (hold while moving a (multi)sector point) 
- `C`... loop cut (hold while adding 2 points to a (multi)sector)
- `X`... extrude (hold while adding a point to a (multi)sector)