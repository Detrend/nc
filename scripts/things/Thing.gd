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
		if val == Things.PlacementMode.Nested and (is_inside_tree() and not(get_parent() is Thing)):
			ErrorUtils.report_error("Cannot assign Nested placement to a Thing that isn't nested!")
			return
		placement_mode = val
	
## Height to offset from the parent [Sector]'s height, in a manner determined by [member placement_mode]
@export var height_offset : float = 0.1

## Unique value assigned at the start of export process. Other objects can refer to this [Thing] by it
var _export_tag : int = -1

const RAW_DATA_EXPORT_CATEGORY :String = "rawdata"

## @brief Section in the level's JSON file where this Thing will be listed. 
## 		Typically should be a unique string for each Thing type.
func get_export_category()->String: return "misc"


## @brief Append custom properties that will be contained in the level's JSON entry for this Thing
##
## @param _s Sector where this thing is placed
## @param _output JSON object describing this Thing, append additional properties here
func custom_export(_s: Sector, _output: Dictionary)->void:
	_output['tag'] = _export_tag

## @c true IFF this Thing is not nested
## Nested [Things] get their position computed based on their parent [Thing]'s position.
## Assume we e.g. want to create an easily placable candle prefab -
##  we would place a candle-sprite [Prop] at the root, and a nested point light source offseted to the position of the candle's tip
func is_standalone()->bool:
	return not ((get_parent() is Thing) and placement_mode == Things.PlacementMode.Nested)

## Get full name identifying this [Thing] in the level hierarchy
func get_full_name()->String: return NodeUtils.get_full_name(self)



static func get_all_standalone_things(root: Node, thing_type : Variant = Thing, ret: Array[Thing] = [])->Array[Thing]:
	ret = NodeUtils.get_children_by_predicate(root, func(n:Node)->bool: return is_instance_of(n, thing_type) and (n as Node2D).is_visible_in_tree() and (n as Thing).is_standalone(), ret, NodeUtils.LOOKUP_FLAGS.RECURSIVE)
	return ret

static func get_all_things(root: Node, thing_type : Variant = Thing, ret: Array[Thing] = [])->Array[Thing]:
	ret = NodeUtils.get_children_by_predicate(root, func(n:Node)->bool: return is_instance_of(n, thing_type) and (n as Node2D).is_visible_in_tree(), ret, NodeUtils.LOOKUP_FLAGS.RECURSIVE)
	return ret
