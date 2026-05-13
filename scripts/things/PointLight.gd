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

@export_group("Animation")
## Animate the light's intensity over time
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var should_animate : bool = false

## Curve over <0;1> interval determining the light's intensity over time
@export var curve : Curve = Curve.new()
## How many steps should the animation have - increase to make the animation more smooth and detailed
@export_range(0.0, 24.0) var curve_quantization : int = 24
## How long does the single cycle of the animation take
@export var cycle_duration_seconds : float = 1.0
@export var cycle_offset_start_seconds : float = 0.0
@export_range(0.0, 1.0) var cycle_offset_range : float = 0.0

func get_export_category()->String: return "point_lights"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output['color'] = [color.r, color.g, color.b, color.a]
	output['intensity'] = intensity
	
	output['radius'] = radius
	output['falloff'] = falloff
	if should_animate:
		output['light_string'] = _curve_to_light_string(curve, curve_quantization)
		output['cycle_length'] = cycle_duration_seconds
		output['cycle_offset'] = cycle_offset_start_seconds + NodeUtils.compute_node_hash01(self) * cycle_offset_range * cycle_duration_seconds  

static func _curve_to_light_string(curve: Curve, point_count : int = 24)->String:
	const RANGE = "zyxwvutsrqponmlkjihgfedcba"
	var ret : String = ""
	for i in point_count:
		var t : float = curve.min_domain + ((float(i) / point_count) * curve.get_domain_range())
		var value := (curve.sample(t) / curve.get_value_range()) - curve.min_value
		assert( 0.0 <= value and value <= 1.0)
		var idx := value * (RANGE.length() - 1)
		ret += RANGE[idx]
	return ret
