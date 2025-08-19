class_name TextUtils

static func substring(s: String, begin: int, end_exclusive: int)->String:
	return s.substr(begin, end_exclusive - begin)

static func recursive_array_tostring(arr: Array, separator :String = ", ", stringifier : Callable = Callable())->String:
	var ret : String = "["
	var is_first :bool = true
	for item in arr:
		if! is_first: ret += separator
		is_first = false
		if item is Array:
			ret += recursive_array_tostring(item, separator, stringifier)
		else:
			if stringifier:
				ret += stringifier.call(item)
			else:
				ret += str(item)
		
	return ret + "]"
	

static func extract_file_name_from_path(path: String)->String:
	var dot_idx = path.rfind('.')
	var slash_idx = path.rfind('/') + 1 # if not found, this gets us to 0
	if dot_idx < slash_idx: dot_idx = path.length() - 1
	return substring(path, slash_idx, dot_idx)

static func vec2_to_array(v: Vector2)->PackedFloat32Array:
	return [v.x, v.y]
	
static func vec3_to_array(v: Vector3)->PackedFloat32Array:
	return [v.x, v.y, v.z]
	
static func vec2_from_array(arr: PackedFloat32Array)->Vector2:
	return Vector2(arr[0],arr[1])
	
static func vec3_from_array(arr: PackedFloat32Array)->Vector3:
	return Vector3(arr[0],arr[1],arr[2])
