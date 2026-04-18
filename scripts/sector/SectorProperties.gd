## Carries configuration of a specific [Sector] like its floor/ceiling heights and texturing configuration.
extends Resource
class_name SectorProperties

## Default floor height of this sector, can be changed during runtime through [SectorAltConfig]
@export var floor_height : float = 0.0
## Default ceiling height of this sector, can be changed during runtime through [SectorAltConfig]
@export var ceiling_height : float = 1.5

## Material used for texturing this sector
@export var material: SectorMaterial = load("res://materials/default_texture.tres")


@export_group("Texturing")

## Offset for floor and ceiling textures applied to this specific sector
@export var texturing_offset : Vector2 = Vector2.ZERO
## Rotation, added to floor and ceiling texture's default rotations when applied to this specific sector
@export_range(0.0, 360.0) var texturing_rotation : float = 0.0

## Offset for wall textures applied to this specific sector. Gets added to the wall-specific texturing offsets that might be applied by [WallTextureOverride] etc.
@export var wall_texturing_offset : Vector2 = Vector2.ZERO
## Rotation, added to wall textures' default rotations when applied to this specific sector
@export_range(0.0, 360.0) var wall_texturing_rotation : float = 0.0
