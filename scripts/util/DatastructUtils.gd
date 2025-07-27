extends Object
class_name DatastructUtils

const HASHSET_PLACEHOLDER_VALUE:bool = true;

static func add_to_list(list, value)->Array:
	if !list: 
		return [value]
	list.append(value)
	return list;


static func _default_compare_lt(a,b): return a<b;

static func find_min(list, selector: Callable, compare_lt: Callable = Callable()):
	if !compare_lt: compare_lt = func(a,b): return a<b;
	var is_first_iteration := true;
	var min = null;
	var min_comparable = null;
	
	for val in list:
		var comparable = selector.call(val);
		if is_first_iteration || compare_lt.call(comparable, min_comparable):
			min = val;
			min_comparable = comparable;
		is_first_iteration = false
	
	return min;

class Wrapper:
	var value;
	
	func _init(value)->void:
		self.value = value


static func string_concat(list, separator = ", ")->String:
	var ret := ""
	var is_first_iteration := true
	for e in list:
		if ! is_first_iteration:
			ret += separator
		is_first_iteration = false
		ret += str(e)
	return ret
