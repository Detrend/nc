## Spherical point light source
@tool
class_name PointLight
extends Thing

## Color of the light
@export var color : Color = Color.WHITE
## Intensity of the light
@export var intensity : float = 1.0

## Radius where the light can affect stuff 
@export var radius : float = 3.0
## How fast the light's intensity fades based on distance from its center
@export var falloff : float = 1.0


func get_export_category()->String: return "point_lights"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output['color'] = [color.r, color.g, color.b, color.a]
	output['intensity'] = intensity
	
	output['radius'] = radius
	output['falloff'] = falloff
	
	
