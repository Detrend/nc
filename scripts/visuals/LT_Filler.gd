@tool
class_name LT_Filler
extends ILayeredTextureEntry

@export var texture : ITextureDefinition


func get_height(total_height: float, available_height: float, _texturing_interval : Vector2, _ctx : TexturingContext)->float:
	return available_height


func get_texture()->ITextureDefinition:
	return texture
