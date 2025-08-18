@tool
class_name LT_Entry
extends ILayeredTextureEntry

enum HeightMode{
	Fixed, TotalPercent, AvailablePercent, Min_Fixed_TotalPercent, Min_Fixed_AvailablePercent, Max_Fixed_TotalPercent, Max_Fixed_AvailablePercent
}


@export var texture : ITextureDefinition

@export_range(0.0, 1.0) var anchor_point : float

@export var height_mode : HeightMode
@export var height_fixed : float
@export_range(0.0, 1.0) var height_percent : float

func get_height(total_height: float, available_height: float)->float:
	var ret :float = 0.0
	if height_mode == HeightMode.Fixed:
		ret = height_fixed
	elif height_mode == HeightMode.TotalPercent:
		ret = total_height * height_percent
	elif height_mode == HeightMode.AvailablePercent:
		ret = available_height * height_percent
	elif height_mode == HeightMode.Min_Fixed_TotalPercent:
		ret = min(height_fixed, total_height * height_percent)
	elif height_mode == HeightMode.Min_Fixed_AvailablePercent:
		ret = min(height_fixed, available_height * height_percent)
	elif height_mode == HeightMode.Max_Fixed_TotalPercent:
		ret = max(height_fixed, total_height * height_percent)
	elif height_mode == HeightMode.Max_Fixed_AvailablePercent:
		ret = max(height_fixed, available_height * height_percent)
	return ret


func get_texture()->ITextureDefinition:
	return texture
