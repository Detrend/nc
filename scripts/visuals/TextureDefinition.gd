@tool
class_name TextureDefinition
extends ITextureDefinition

@export var preview : Texture
@export var _id : String
@export var scale : float = 1.0
@export_range(0.0, 360.0) var rotation : float = 0.0
@export_range(1, 32) var tile_rotations_count : int = 1
@export_range(0.0, 360.0) var tile_rotation_increment : float = 90.0
@export var should_show : bool = true

var id: String:
	get: 
		if preview:
			var ret := TextUtils.extract_file_name_from_path(preview.resource_path)
			if not ret.is_empty(): return ret
		return _id


func append_info(out: Array[Dictionary], begin_height: float, end_height: float, ctx: TexturingContext)->void:
	var info : Dictionary = {}
	info["show"] = should_show
	if should_show:
		info["id"] = id
		info["scale"] = scale
		var base_rotation_deg :float = self.rotation
		var custom_rotation_deg :float = ctx.target_sector.data.wall_texturing_rotation if ctx.subject_type == ITextureDefinition.TexturingSubjectType.Wall else ctx.target_sector.data.texturing_rotation
		info["rotation"] = deg_to_rad(base_rotation_deg + custom_rotation_deg)
		var offset := ctx.target_sector.data.wall_texturing_offset if (ctx.subject_type == ITextureDefinition.TexturingSubjectType.Wall) else ctx.target_sector.data.texturing_offset
		info["offset"] = [offset.x, offset.y]
		info["tile_rotations_count"] = tile_rotations_count if tile_rotations_count else 0
		info["tile_rotation_increment"] = deg_to_rad(tile_rotation_increment)
		
	if ctx.subject_type == TexturingSubjectType.Wall:
		info["begin_height"] = begin_height
		info["end_height"] = end_height
	out.append(info)
