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
