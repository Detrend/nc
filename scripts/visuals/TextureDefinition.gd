## Single texture
##
## Can be applied to any level surface (floor,ceiling,wall) or used as a part of more complex wall material.
##
## All [TextureDefinition] [Resource]s reside in the `material_textures` directory
@tool
class_name TextureDefinition
extends ITextureDefinition

## Preview image of this texture. If no [member _id] is explicitly provided, use the name of this texture file.
@export var preview : Texture
## Identifier to lookup the texture in-game. If not provided, use name of the [member preview] texture file.
@export var _id : String
## Identifier of the alternative texture to be used in the triggered state. Only relevant for button texutres.
@export var id_triggered : String
## The bigger number, the more zoomed-in the texture is. 
@export var scale : float = 1.0
## Base rotation (in degrees) of the texture. Gets added to the randomized tile rotation.
@export_range(0.0, 360.0) var rotation : float = 0.0
## How many rotation options there are when doing tile rotation randomization.
@export_range(1, 32) var tile_rotations_count : int = 1
## Increment in degrees for determining the rotation options.
##
## E.g. for [member tile_rotations_count] = 2, [member tile_rotation_increment] = 90, we will randomly choose between rotations {0deg, 90deg} for each tile
## E.g. for [member tile_rotations_count] = 5, [member tile_rotation_increment] = 10, we will randomly choose between rotations {0deg, 10deg, 20deg, 30deg, 40deg} for each tile
@export_range(0.0, 360.0) var tile_rotation_increment : float = 90.0
## When [constant false] do not draw any texture, but just empty space (used for outdoors spaces)
@export var should_show : bool = true

## Identifier of the texture
var id: String:
	get: 
		if self.preview:
			var ret := TextUtils.extract_file_name_from_path(self.preview.resource_path)
			if not ret.is_empty(): return ret
		return _id


## Generate a [TexturingResult] covering the whole assigned space by this texture
func resolve(out: TexturingResult, begin_height: float, end_height: float, ctx: TexturingContext)->void:
	var info := out.add_entry()
	info.show = should_show
	if should_show:
		info.id = self.id
		if id_triggered and (not id_triggered.is_empty()): info.id_triggered = id_triggered
		info.scale = scale
		var base_rotation_deg :float = self.rotation
		var custom_rotation_deg :float = ctx.target_sector.data.wall_texturing_rotation if ctx.subject_type == TexturingContext.TexturingSubjectType.Wall else ctx.target_sector.data.texturing_rotation
		info.rotation_deg = base_rotation_deg + custom_rotation_deg
		var offset := ctx.target_sector.data.wall_texturing_offset if (ctx.subject_type == TexturingContext.TexturingSubjectType.Wall) else ctx.target_sector.data.texturing_offset
		info.offset = offset
		info.tile_rotations_count = tile_rotations_count if tile_rotations_count else 0
		info.tile_rotation_increment_deg = tile_rotation_increment
		
	if ctx.subject_type == TexturingContext.TexturingSubjectType.Wall:
		info.is_wall = true
		info.begin_height = begin_height
		info.end_height = end_height

## Debug name - based on the texture's id 
func _to_string() -> String: return ("" if should_show else "void-") + "tex(%s)"%id
