@tool
class_name WallRule
extends IWallRule

enum PlacementType {
	None = 0,
	Wall = 4, 
	Floor = 8,
	Ceiling = 16,
	HoleBorder = 2,
	Border = 1, 
	Any = Border | Wall | HoleBorder
}

@export var priority : int = 0
@export_flags("Wall:4", "Floor:8", "Ceiling:16", "HoleBorder:2",  "All:31") var _placement_type : int = (PlacementType.Any as int)

@export var wall_length_range : Vector2 = Vector2(-INF, INF):
	get: return Vector2(min(wall_length_range.x, wall_length_range.y), max(wall_length_range.x, wall_length_range.y))
	set(val): wall_length_range = val


var placement_type : PlacementType:
	get: return _placement_type as PlacementType

@export var texture : TextureDefinition


func get_priority()->int:
	return priority
	
func get_texture()->ITextureDefinition:
	return texture
