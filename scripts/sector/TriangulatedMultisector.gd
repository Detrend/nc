@tool
class_name TriangulatedMultisector
extends EditablePolygon

## Perform the triangulation (Alt+T)
@export_tool_button("Triangulate") var triangulate_tool_button = do_triangulate

## Do debug stuff
@export var debug : bool = false
@export var aggressive : bool = false

@export var merge_holes : bool = false

@export var additional_holes : Array[Node]

@export var only_manual_triangulation : bool = false

@export var data : SectorProperties = SectorProperties.new()

@export_tool_button("Bake") var dissolve_tool_button = do_dissolve

func get_own_prefab()->Resource:
	return Level.TRIANGULATED_MULTISECTOR_PREFAB

var _generated_parent : Node2D:
	get: return $Generated

#func _selected_update(_selected_list: Array[Node])->void:
#	super._selected_update(_selected_list)
#	if _selected_list.find(self) == -1:
#		triangulate_on_next_frame()

func on_editing_start()->void:
	super.on_editing_start()
	if only_manual_triangulation: return
	do_clear()

func on_editing_finish(_start_was_called_first : bool)->void:
	super.on_editing_finish(_start_was_called_first)
	triangulate_on_next_frame()


func on_descendant_editing_start(_ancestor: Node)->void:
	super.on_descendant_editing_start(_ancestor)
	if only_manual_triangulation: return
	do_clear()

func on_descendant_editing_finish(_ancestor: Node, _start_was_called_first: bool):
	super.on_descendant_editing_finish(_ancestor, _start_was_called_first)
	triangulate_on_next_frame()

func _ready() -> void:
	do_triangulate()

var has_triangulation_request: bool = false
func triangulate_on_next_frame()->void:
	if only_manual_triangulation: return
	if has_triangulation_request: return
	has_triangulation_request = true
	await _level.every_frame_signal
	if not has_triangulation_request: return
	has_triangulation_request = false
	do_triangulate()
		

func do_triangulate()->void:
	do_clear()

	var generated_parent := _generated_parent
	var hole_nodes : Array[EditablePolygon] = EditablePolygon.get_all_editable(self)
	var holes : Array[PackedVector2Array] = []
	for hole_node in hole_nodes:
		if hole_node.is_visible_in_tree() and hole_node.is_editable:
			holes.append(hole_node.get_point_positions())
	if additional_holes and !additional_holes.is_empty():
		for hole_root in additional_holes:
			if hole_root == null: continue
			for hole_node in EditablePolygon.get_all_editable(hole_root):
				if hole_node.is_visible_in_tree() and hole_node.is_editable:
					holes.append(hole_node.get_point_positions())

	#var merged_holes : Array[PackedVector2Array]
	#var merged_hole_holes : Array[PackedVector2Array]
	#GeometryUtils.merge_polygons(holes, merged_holes, merged_hole_holes)
	#var hole_holes_begin = merged_holes.size()
	#merged_holes.append_array(merged_hole_holes)

	var computed_segments := GeometryUtils.polygon_to_convex_segments(self.get_point_positions(), holes, debug, aggressive, merge_holes)
	var counter : int = 0
	for segment in computed_segments:
		#print("clockwise: {0}".format([Geometry2D.is_polygon_clockwise(segment)]))
		var added :Sector = NodeUtils.instantiate_child(generated_parent, preload("res://prefabs/Sector.tscn")) as Sector
		added.global_position = Vector2.ZERO
		added.polygon = segment
		added.is_editable = only_manual_triangulation#false
		added.data = self.data
		#if counter >= hole_holes_begin:
		#	added.data = added.data.duplicate()
		#	added.data.floor_height = 42.24
		added.z_index = -10
		added.name = "_" + str(counter)
		counter += 1

	await _level.every_frame_signal	
	for child in generated_parent.get_children():
		(child as Sector)._update_visuals() 	

func _update_visuals() -> void:
	if _generated_parent.get_child_count() > 0:
		for s in _generated_parent.get_children(): (s as Sector)._update_visuals()
	else:
		do_triangulate()
		 
func do_clear()->void:
	var generated_parent : Node2D = $Generated
	for ch in generated_parent.get_children(): generated_parent.remove_child(ch)

func do_dissolve()->void:
	do_triangulate()
	
	var generated_parent := _generated_parent

	var generated_sectors : Array[Sector] 
	generated_sectors = NodeUtils.get_children_of_type(generated_parent, Sector, generated_sectors)
	var manual_children : Array[Node]
	manual_children = NodeUtils.get_children_by_predicate(self, func(n: Node): return n != generated_parent, manual_children, NodeUtils.LOOKUP_FLAGS.NONE)

	var new_name := self.name + "-Gen"
	var unre:EditorUndoRedoManager= get_undo_redo()

	unre.create_action("Multisector Dissolve")
	var new_parent := NodeUtils.add_do_undo_child(unre, self.get_parent(), Node2D.new(), self.get_index()) as Node2D
	new_parent.position = self.position
	new_parent.name = new_name
	unre.add_do_method(new_parent, 'set_owner', get_tree().edited_scene_root)
	for s in generated_sectors:
		generated_parent.remove_child(s)
		new_parent.add_child(s)
		unre.add_do_method(s, 'set_owner', get_tree().edited_scene_root)
		s.is_editable = true
	#for ch in manual_children:
	#	NodeUtils.relink_do_undo_node(unre, ch, new_parent, -1, get_tree().edited_scene_root) 
	if config.dissolve_removes_self:
		pass#NodeUtils.remove_do_undo_node(unre, self)

	print("root: {0}".format([get_tree().edited_scene_root]))

	unre.commit_action()
	#NodeUtils.set_selection([new_parent])
