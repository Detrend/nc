## Namespace containing common [Thing]-related stuff
class_name Things

## How to compute a [Thing]'s vertical placement in the game world
enum PlacementMode{
	## Add [member Thing.height_offset] to the containing [Sector]'s [member Sector.floor_height]
	Floor = 0, 
	## Subtract [member Thing.height_offset] from the containing [Sector]'s [member Sector.ceiling_height]
	Ceiling, 
	## Use [member Thing.height_offset] as absolute Y value
	Absolute, 
	## Add [member Thing.height_offset] to the parent [Thing]'s vertical placement
	Nested
}
