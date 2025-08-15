extends Resource
class_name SectorMaterial

@export var preview : Texture2D
@export var preview_scale : float = 1.0
@export_range(0.0, 360.0) var preview_rotation : float = 0.0


@export_category("Wall")
@export var wall : TextureDefinition
@export var wall_priority : int = 0
@export var wall_rules : Array[WallRule]

@export_category("Floor")
@export var floor : TextureDefinition

@export_category("Ceiling")
@export var ceiling : TextureDefinition
