## Definition of the ambient light.
## Its position is insignificant as it globally affects the whole level.
@tool
class_name AmbientLight
extends Thing

## Intensity of the ambient light. Currently the light has always white color.
@export_range(0.0, 1.0, 0.05) var intensity : float = 1.0

func get_export_category()->String: return "ambient_lights"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output['intensity'] = intensity
	
	
