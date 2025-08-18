class_name ExtrudedTexture
extends ITextureDefinition

@export var texture : ITextureDefinition

@export var begin_up_direction : Vector3 = Vector3.ZERO
@export var   end_up_direction : Vector3 = Vector3.ZERO
@export var begin_down_direction : Vector3 = Vector3.ZERO
@export var   end_down_direction : Vector3 = Vector3.ZERO

func append_info(out: Array[Dictionary], begin_height: float, end_height: float, ctx: TexturingContext)->void:
	var idx := out.size()
	append_info(out, begin_height, end_height, ctx)
	while idx < out.size():
		out[idx].get_or_add('begin_up_direction', begin_up_direction)
		out[idx].get_or_add('end_up_direction', end_up_direction)
		out[idx].get_or_add('begin_down_direction', begin_down_direction)
		out[idx].get_or_add('end_down_direction', end_down_direction)
		idx += 1
