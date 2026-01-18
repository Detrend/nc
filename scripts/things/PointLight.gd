@tool
class_name PointLight
extends Thing

@export var color : Color = Color.WHITE
@export var intensity : float = 1.0

@export var radius : float = 3.0
@export var falloff : float = 1.0


func get_export_category()->String: return "point_lights"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output['color'] = [color.r, color.g, color.b, color.a]
	output['intensity'] = intensity
	
	output['radius'] = radius
	output['falloff'] = falloff
	
	
