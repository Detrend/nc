@tool
extends Node2D
class_name Thing


@export var placement_mode: Things.PlacementMode = Things.PlacementMode.Floor
@export var height_offset : float = 0.1

@export_group("Nested")
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var is_nested:bool = false


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
func is_standalone()->bool:
	return not ((get_parent() is Thing) and is_nested)
