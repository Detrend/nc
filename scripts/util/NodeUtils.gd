extends Object
class_name NodeUtils

enum LOOKUP_FLAGS{
	NONE=0,
	RECURSIVE=1,
	REQUIRED=2
}
	
static func get_descendant_of_type(node: Node, child_type, flags := LOOKUP_FLAGS.RECURSIVE):
		return get_child_of_type(node, child_type, flags | LOOKUP_FLAGS.RECURSIVE)


static func get_child_of_type(node: Node, child_type, flags := LOOKUP_FLAGS.NONE):
	if node:
		for i in range(node.get_child_count()):
			var child = node.get_child(i)
			if is_instance_of(child, child_type):
				return child
			if flags & LOOKUP_FLAGS.RECURSIVE:
				var child_node = get_child_of_type(child, child_type, flags)
				if child_node:
					return child_node
	if flags & LOOKUP_FLAGS.REQUIRED:
		ErrorUtils.report_error("Did not find required component of type '{0}' on node '{1}'"
			.format([child_type.get_path(),node.get_path()]));
	return null


static func get_ancestor_of_type(node: Node, parent_type, flags := LOOKUP_FLAGS.NONE):
	while node:
		if is_instance_of(node, parent_type):
			return node;
		node = node.get_parent()
		
	if flags & LOOKUP_FLAGS.REQUIRED:
		ErrorUtils.report_error("Did not find required ancestor of type '{0}' of node '{1}'"
			.format([parent_type.get_path(),node.get_path()]));
	return null;
		
static func get_ancestor_component_of_type(node: Node, component_type, flags := LOOKUP_FLAGS.NONE):
	if !node: return null;
	while node.get_parent():
		var found = get_sibling_of_type(node, component_type)
		if found: return found
		node = node.get_parent()
		
	if flags & LOOKUP_FLAGS.REQUIRED:
		ErrorUtils.report_error("Did not find required component of type '{0}' in ancestors of node '{1}'"
			.format([component_type.get_path(),node.get_path()]));
	return null;

static func get_descendants_of_type(node: Node, child_type, list: Array = [], flags := LOOKUP_FLAGS.RECURSIVE):
		return get_children_of_type(node, child_type, list, flags | LOOKUP_FLAGS.RECURSIVE)

static func get_children_by_predicate(node: Node, predicate: Callable, list: Array = [], flags := LOOKUP_FLAGS.NONE):
	if !node: return [];
	
	for i in range(node.get_child_count()):
		var child = node.get_child(i)
		if predicate.call(child):
			list.append(child)
		if flags & LOOKUP_FLAGS.RECURSIVE:
			list = get_children_by_predicate(child, predicate,list, flags & ~LOOKUP_FLAGS.REQUIRED)
			
	if list.is_empty() && flags & LOOKUP_FLAGS.REQUIRED:
		ErrorUtils.report_error("Did not find required component on node '{0}'"
			.format([node.get_path()]));
	return list
	

static func get_children_of_type(node: Node, child_type, list :Array = [], flags := LOOKUP_FLAGS.NONE):
	if !node: return [];
	
	for i in range(node.get_child_count()):
		var child = node.get_child(i)
		if is_instance_of(child, child_type):
			list.append(child)
		if flags & LOOKUP_FLAGS.RECURSIVE:
			list = get_children_of_type(child, child_type,list, flags & ~LOOKUP_FLAGS.REQUIRED)
			
	if list.is_empty() && flags & LOOKUP_FLAGS.REQUIRED:
		ErrorUtils.report_error("Did not find required component of type '{0}' on node '{1}'"
			.format([child_type.get_path(),node.get_path()]));
	return list

static func get_sibling_of_type(node: Node, child_type, flags := LOOKUP_FLAGS.NONE):
	if(!node): return null;
	return get_child_of_type(node.get_parent(), child_type, flags);
	
static func get_siblings_of_type(node: Node, child_type, list :Array = [], flags := LOOKUP_FLAGS.NONE):
	if(!node): return [];
	return get_children_of_type(node.get_parent(), list, child_type, flags);
	
	
static func is_the_only_selected_node(node: Node)->bool:
	var selection := EditorInterface.get_selection().get_selected_nodes()
	return selection.size() == 1 and selection[0] == node
	
static func get_selected_nodes_of_type(type: Variant, ret : Array = [])->Array:
	var selection := EditorInterface.get_selection().get_selected_nodes()
	for n in selection:
		if is_instance_of(n, type):
			ret.append(n)
	return ret

static var _counter: int = 0;
static func get_unique_id()->int: 
	_counter += 1;
	return _counter;

static func instantiate_child(parent: Node, prefab: Resource)->Node:
	var ret :Node = prefab.instantiate()
	parent.add_child(ret)
	ret.owner = parent.get_tree().edited_scene_root
	return ret
