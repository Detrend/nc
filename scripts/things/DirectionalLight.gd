@tool
class_name DirectionalLight
extends Thing

@export var direction : Vector3 = Vector3(0, 0, -1)
@export var color : Color = Color.WHITE
@export_range(0.0, 1.0, 0.05) var intensity : float = 1.0


func get_export_category()->String: return "directional_lights"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output['direction'] = TextUtils.vec3_to_array(direction)
	output['color'] = [color.r, color.g, color.b, color.a]
	output['intensity'] = intensity
	
	
