extends Resource
class_name SectorProperties

@export var floor_height : float = 0.0
@export var ceiling_height : float = 1.5

@export var material: SectorMaterial = preload("res://materials/default_texture.tres")


@export_group("Texturing")

@export var texturing_offset : Vector2 = Vector2.ZERO
@export_range(0.0, 360.0) var texturing_rotation : float = 0.0

@export var wall_texturing_offset : Vector2 = Vector2.ZERO
@export_range(0.0, 360.0) var wall_texturing_rotation : float = 0.0
