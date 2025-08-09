extends Resource
class_name SectorMaterial

@export var preview : Texture2D
@export var preview_scale : float = 1.0


@export_group("Wall")
@export var show_wall : bool = true
@export var wall_texture : String
@export var wall_scale : float = 1.0
@export var wall_priority : int = 0
@export var wall_rules : Array[WallRule]

@export_group("Floor")
@export var show_floor : bool = true
@export var floor_texture : String
@export var floor_scale : float = 1.0

@export_group("Ceiling")
@export var show_ceiling : bool = true
@export var ceiling_texture : String
@export var ceiling_scale : float = 1.0
