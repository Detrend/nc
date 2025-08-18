@tool
class_name TextureDefinition
extends ITextureDefinition

@export var preview : Texture
@export var _id : String
@export var scale : float = 1.0
@export_range(0.0, 360.0) var rotation : float = 0.0
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
		info["rotation"] = rotation
	if ctx.subject_type == TexturingSubjectType.Wall:
		info["begin_height"] = begin_height
		info["end_height"] = end_height
	out.append(info)
