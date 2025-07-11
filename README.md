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
- The editor does NOT enforce convexity in any way, user is 100% responsible for that.
- Editor makes automatically sure that the points are ordered clockwise and that there are not multiple points at the exactly same position.
- It seems as a really good idea to enable Godot's grid-snap while configuring the sector points.
- If multiple sectors share a point - hold `Q` while moving that point to move the same point of the other sectors as well.
- `Hide` a sector to exclude it from level export
- Placing portals... should be intuitive, only one portal per sector is currently permitted. _One-sided portals are available for experimentation, but they assert-crash the game._
- `floor`, `ceiling`... visualized by sector's color - see related properties in `config.tres`, switch between `floor`/`ceiling` visualization via `Level`'s `coloring_mode` property 