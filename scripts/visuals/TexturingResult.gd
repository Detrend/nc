class_name TexturingResult
extends RefCounted

class Entry:
	var is_wall : bool
	var begin_height : float
	var end_height : float
	
	var show : bool
	var id : String
	var scale : float
	var rotation_deg : float
	var tile_rotations_count : int
	var tile_rotation_increment_deg : int
	var offset : Vector2

	var is_extruded : bool
	var extrude_use_absolute_directions : bool
	var begin_up_direction : Vector3
	var end_up_direction : Vector3
	var begin_down_direction : Vector3
	var end_down_direction : Vector3

	func export(out: Dictionary = {})->Dictionary:
		out["show"] = show
		if show:
			out["id"] = id
			out["scale"] = scale 
			out["rotation"] = deg_to_rad(rotation_deg)
			out["offset"] = [offset.x, offset.y]
			out["tile_rotations_count"] = tile_rotations_count
			out["tile_rotation_increment"] = deg_to_rad(tile_rotation_increment_deg)
		if is_wall:
			out["begin_height"] = begin_height
			out["end_height"] = end_height
		if is_extruded:
			out["begin_up_direction"] = TextUtils.vec3_to_array(begin_up_direction)
			out["end_up_direction"] = TextUtils.vec3_to_array(end_up_direction)
			out["begin_down_direction"] = TextUtils.vec3_to_array(begin_down_direction)
			out["end_down_direction"] = TextUtils.vec3_to_array(end_down_direction)
			out["absolute_directions"] = extrude_use_absolute_directions
		return out

var entries : Array[Entry]




func export(out: Array[Dictionary] = [])->Array[Dictionary]:
	for e in entries:
		out.append(e.export())
	return out
