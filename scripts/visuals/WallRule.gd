extends Resource
class_name WallRule

enum PlacementType {
	None = 0,
	Border = 1, 
	HoleBorder = 2,
	Wall = 4, 
	Any = Border | Wall | HoleBorder
}

@export var priority : int = 0
@export_flags("Border:1", "HoleBorder:2", "Wall:4", "All:7") var _placement_type : int = (PlacementType.Any as int)

@export var wall_length_range : Vector2 = Vector2(-INF, INF):
	get: return Vector2(min(wall_length_range.x, wall_length_range.y), max(wall_length_range.x, wall_length_range.y))
	set(val): wall_length_range = val


var placement_type : PlacementType:
	get: return _placement_type as PlacementType

@export var texture : TextureDefinition
