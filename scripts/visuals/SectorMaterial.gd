extends Resource
class_name SectorMaterial

@export var preview : Texture2D
@export var preview_scale : float = 1.0
@export_range(0.0, 360.0) var preview_rotation : float = 0.0


@export_category("Wall")
@export var wall : TextureDefinition
@export var show_wall : bool = true
@export var wall_texture : String
@export var wall_scale : float = 1.0
@export_range(0.0, 360.0) var wall_rotation : float = 0.0
@export var wall_priority : int = 0
@export var wall_rules : Array[WallRule]

@export_category("Floor")
@export var floor : TextureDefinition
@export var show_floor : bool = true
@export var floor_texture : String
@export var floor_scale : float = 1.0
@export_range(0.0, 360.0) var floor_rotation : float = 0.0

@export_category("Ceiling")
@export var ceiling : TextureDefinition
@export var show_ceiling : bool = true
@export var ceiling_texture : String
@export var ceiling_scale : float = 1.0
@export_range(0.0, 360.0) var ceiling_rotation : float = 0.0
