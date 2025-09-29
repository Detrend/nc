@tool
extends Thing
class_name Entity

func get_export_category()->String: return "entities"


func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output["is_player"] = false
	output["forward"] = TextUtils.vec3_to_array(_get_forward_direction())
	
func _get_forward_direction()->Vector3:
	var rot := Vector2(0.0, -1.0).rotated(self.global_rotation)
	return Vector3(rot.x, rot.y, 0.0)
