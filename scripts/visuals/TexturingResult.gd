@tool
class_name TexturingResult
extends RefCounted

class Entry:
	var is_wall : bool = false
	var begin_height : float
	var end_height : float
	
	var show : bool = false
	var id : String
	var scale : float # not yet multiplied by level's texturing scale
	var rotation_deg : float
	var tile_rotations_count : int
	var tile_rotation_increment_deg : float
	var offset : Vector2

	var is_extruded : bool = false
	var extrude_use_absolute_directions : bool
	# these are already premultiplied by the level's texturing scale - raw blackbox data that shouldn't be processed further in any way
	var begin_up_direction : Vector3
	var end_up_direction : Vector3
	var begin_down_direction : Vector3
	var end_down_direction : Vector3

	func export(out: Dictionary, ctx: TexturingContext)->Dictionary:
		out["show"] = show
		if show:
			out["id"] = id
			out["scale"] = scale * ctx.level.export_texture_scale
			out["rotation"] = deg_to_rad(rotation_deg)
			out["offset"] = [offset.x, offset.y]
			out["tile_rotations_count"] = tile_rotations_count
			out["tile_rotation_increment"] = deg_to_rad(tile_rotation_increment_deg)
		if is_wall:
			out["begin_height"] = begin_height  * ctx.export_scale.z
			out["end_height"] = end_height * ctx.export_scale.z
		if is_extruded:
			out["begin_up_direction"] = TextUtils.vec3_to_array(begin_up_direction)
			out["end_up_direction"] = TextUtils.vec3_to_array(end_up_direction)
			out["begin_down_direction"] = TextUtils.vec3_to_array(begin_down_direction)
			out["end_down_direction"] = TextUtils.vec3_to_array(end_down_direction)
			out["absolute_directions"] = extrude_use_absolute_directions
		return out

	func clone() -> Entry:
		var ret := Entry.new()
		for property in ret.get_property_list():
			var name :String = property.name
			ret.set(name, self.get(name))
		return ret

var entries : Array[Entry]


func add_entry()->Entry:
	var ret := Entry.new()
	entries.append(ret)
	return ret


func export(out: Array[Dictionary], ctx: TexturingContext)->Array[Dictionary]:
	for e in entries:
		out.append(e.export({}, ctx))
	return out
