## Generic object that can be placed in the game world.
##
## Has a position (XY determined from Node2D placement, height computed based on containing [Sector])
##  and custom properties 
@tool
extends Node2D
class_name Thing

## How to compute a [Thing]'s vertical placement in the game world
@export var placement_mode: Things.PlacementMode = Things.PlacementMode.Floor:
	get: return placement_mode
	set(val):
		if val == Things.PlacementMode.Nested && not(get_parent() is Thing):
			ErrorUtils.report_error("Cannot assign Nested placement to a Thing that isn't nested!")
			return
		placement_mode = val
	
## Height to offset from the parent [Sector]'s height, in a manner determined by [member placement_mode]
@export var height_offset : float = 0.1


## @brief Section in the level's JSON file where this Thing will be listed. 
## 		Typically should be a unique string for each Thing type.
func get_export_category()->String: return "misc"


## @brief Append custom properties that will be contained in the level's JSON entry for this Thing
##
## @param _s Sector where this thing is placed
## @param _output JSON object describing this Thing, append additional properties here
func custom_export(_s: Sector, _output: Dictionary)->void:
	pass

## @c true IFF this Thing is not nested
## Nested [Things] get their position computed based on their parent [Thing]'s position.
## Assume we e.g. want to create an easily placable candle prefab -
##  we would place a candle-sprite [Prop] at the root, and a nested point light source offseted to the position of the candle's tip
func is_standalone()->bool:
	return not ((get_parent() is Thing) and placement_mode == Things.PlacementMode.Nested)

## Get full name identifying this [Thing] in the level hierarchy
func get_full_name()->String: return NodeUtils.get_full_name(self)
