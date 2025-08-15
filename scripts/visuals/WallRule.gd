extends Resource
class_name WallRule

enum PlacementType {
	None = 0,
	Border = 1, 
	HoleBorder = 2,
	Wall = 4, 
	Any = Border | Wall | HoleBorder
}

@export_flags("Border", "HoleBorder", "Wall") var _placement_type : int = (PlacementType.Any as int)

@export var wall_length_range : Vector2 = Vector2(-INF, INF):
	get: return Vector2(min(wall_length_range.x, wall_length_range.y), max(wall_length_range.x, wall_length_range.y))
	set(val): wall_length_range = val


var placement_type : PlacementType:
	get: return _placement_type as PlacementType


@export var texture : TextureDefinition
@export var show_wall : bool = true
@export var wall_texture : String
@export var wall_scale : float = 1.0
@export_range(0.0, 360.0) var wall_rotation : float = 0.0
