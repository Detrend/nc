# This file contains functions that operate on strings or help with serialization to text-based formats like e.g. JSON

class_name TextUtils

static func substring(s: String, begin: int, end_exclusive: int)->String:
	return s.substr(begin, end_exclusive - begin)

static func recursive_array_tostring(arr, separator :String = ", ", stringifier : Callable = Callable())->String:
	var ret : String = "["
	var is_first :bool = true
	for item in arr:
		if! is_first: ret += separator
		is_first = false
		if DatastructUtils.is_array_like(item):
			ret += recursive_array_tostring(item, separator, stringifier)
		else:
			if stringifier:
				ret += stringifier.call(item)
			else:
				ret += str(item)
		
	return ret + "]"
	
static func is_valid_path(path: String)->bool:
	var start_idx = path.find("://") + 2
	for c in ["<",">",":","\"","\\","|","?","*"]:
		if path.find(c, start_idx) != -1: return false
	return true

static func extract_file_name_from_path(path: String)->String:
	if not is_valid_path(path):
		ErrorUtils.report_error("invalid path: '{0}'".format([path]))
		return ""
	var dot_idx = path.rfind('.')
	var slash_idx = path.rfind('/') + 1 # if not found, this gets us to 0
	if dot_idx < slash_idx: dot_idx = path.length() - 1
	return substring(path, slash_idx, dot_idx)

static func vec2_to_array(v: Vector2)->PackedFloat32Array:
	return [v.x, v.y]
	
static func vec3_to_array(v: Vector3)->PackedFloat32Array:
	return [v.x, v.y, v.z]
	
static func vec4_to_array(v: Vector4)->PackedFloat32Array:
	return [v.x, v.y, v.z, v.w]
	
static func vec2i_to_array(v: Vector2i)->PackedInt64Array:
	return [v.x, v.y]
	
static func vec3i_to_array(v: Vector3i)->PackedInt64Array:
	return [v.x, v.y, v.z]
	
static func vec4i_to_array(v: Vector4i)->PackedInt64Array:
	return [v.x, v.y, v.z, v.w]
	
static func vec2_from_array(arr: PackedFloat32Array)->Vector2:
	return Vector2(arr[0],arr[1])
	
static func vec3_from_array(arr: PackedFloat32Array)->Vector3:
	return Vector3(arr[0],arr[1],arr[2])

static func sanitize_json_object(root):
	if root is Vector2:	return vec2_to_array(root as Vector2)
	if root is Vector3:	return vec3_to_array(root as Vector3)
	if root is Vector4:	return vec4_to_array(root as Vector4)
	if root is Vector2i:	return vec2i_to_array(root as Vector2i)
	if root is Vector3i:	return vec3i_to_array(root as Vector3i)
	if root is Vector4i:	return vec4i_to_array(root as Vector4i)
	if root is Dictionary:
		var ret : Dictionary = {}
		for k in root:
			ret[sanitize_json_object(k)] = sanitize_json_object(root[k])
		return ret
	if DatastructUtils.is_array_like(root):
		var ret : Array = []
		for item in root:
			ret.append(sanitize_json_object(item))
		return ret
	return root
