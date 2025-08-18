@tool
extends Resource
class_name SectorMaterial

@export var preview : Texture2D
@export var preview_scale : float = 1.0
@export_range(0.0, 360.0) var preview_rotation : float = 0.0

@export_category("Texturing")

@export var wall_default : DefaultWallRule = DefaultWallRule.new()
@export var wall_rules : Array[WallRule]

@export var floor : TextureDefinition

@export var ceiling : TextureDefinition
