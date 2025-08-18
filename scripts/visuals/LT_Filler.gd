class_name LT_Filler
extends ILayeredTextureEntry

@export var texture : ITextureDefinition


func get_height(total_height: float, available_height: float)->float:
	return available_height


func get_texture()->ITextureDefinition:
	return texture