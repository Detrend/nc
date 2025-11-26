@tool
class_name Prop
extends Thing

@export var radius : float
@export var height : float

@export_group("Appearance")
@export var sprite_name : String
@export var direction: Vector3 = Vector3.ZERO
@export var sprite_scale: float = 1.0
@export var sprite_mode: SpriteMode = SpriteMode.MONO
@export var pivot_mode : PivotMode = PivotMode.CENTERED
@export var scaling_mode : ScalingMode = ScalingMode.TEXTURE_SIZE
@export var rotation_mode : RotationMode = RotationMode.ONLY_HORIZONTAL

enum SpriteMode{
	MONO, DIR8
}
enum PivotMode{
	CENTERED, BOTTOM
}
enum ScalingMode{
	FIXED, TEXTURE_SIZE
}
enum RotationMode{
	ONLY_HORIZONTAL, FULL
}

func get_export_category()->String: return "props"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output['radius'] = radius * _s._level.export_scale.x
	output['height'] = height * _s._level.export_scale.z
	
	output['sprite'] = sprite_name
	output['direction'] = TextUtils.vec3_to_array(direction.normalized())
	output['scale'] = sprite_scale * _s._level.export_scale.z
	output['mode'] = int(sprite_mode)
	output['pivot'] = int(pivot_mode)
	output['scaling'] = int(scaling_mode)
	output['rotation'] = int(rotation_mode)
	
	
