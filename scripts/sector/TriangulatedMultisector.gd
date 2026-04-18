## Arbitrary non-convex polygon that automatically triangulates its area by convex [Sectors].
##
## Use this for creating mostly flat areas that have stuff placed inside them (e.g. parkour segments or outdoor areas)
##  or for flat but curved corridors etc. .
## [br]
## You can nest [Sector]s inside it as its children - those will be treated as holes by the triangulation process.
## [br]
## Triangulation isn't perfect and can fail for some more complicated hole placements. 
##  When that happens, try adding more points on the boundary or tweaking the [member merge_holes] and [member aggressive] parameters.
@tool
class_name TriangulatedMultisector
extends EditablePolygon

## Perform the triangulation (Alt+T)
@export_tool_button("Triangulate") var triangulate_tool_button = do_triangulate

## If enabled, do not perform triangulation but just submit the single holed polygon that normally gets passed to triangulation.
@export var debug : bool = false

## If enabled, perform triangulation every time a single hole gets processed instead of just at the end.
## Results in much slower performance and suboptimal triangulation, but sometimes helps to let triangulation not fail.
@export var aggressive : bool = false

## If enabled, try to merge all holes that overlap into a single hole. Sometimes helps to let triangulation not fail. [br]
## Currently there is a bug, that if merged holes completely enclose an empty space (e.g. walls around an area), that empty space will be also considered part of the hole.
@export var merge_holes : bool = false

## Additional subtrees containing [EditablePolygon]s, that aren't parented by this multisector, but should also be treated as holes.
@export var additional_holes : Array[Node]

## If enabled, do not automatically triangulate this sector on input events
@export var only_manual_triangulation : bool = false

## Sector config to be used for all generated sectors
@export var data : SectorProperties = SectorProperties.new()

@export_tool_button("Bake") var dissolve_tool_button = do_dissolve

## Get cannonical prefab for creating instances of [TriangulatedMultisector]
func get_own_prefab()->PackedScene:
	return Level.TRIANGULATED_MULTISECTOR_PREFAB

var _generated_parent : Node2D:
	get: return $Generated

## Editing has started - clear all generated [Sector]s
func on_editing_start()->void:
	super.on_editing_start()
	if only_manual_triangulation: return
	do_clear()

## When editing has finished - perform new triangulation
func on_editing_finish(_start_was_called_first : bool)->void:
	super.on_editing_finish(_start_was_called_first)
	triangulate_on_next_frame()

## User started editing some of our holes - clear all generated [Sector]s
func on_descendant_editing_start(_ancestor: Node)->void:
	super.on_descendant_editing_start(_ancestor)
	if only_manual_triangulation: return
	do_clear()

## User just finished editing some of our holes - perform new triangulation
func on_descendant_editing_finish(_ancestor: Node, _start_was_called_first: bool):
	super.on_descendant_editing_finish(_ancestor, _start_was_called_first)
	triangulate_on_next_frame()

func _ready() -> void:
	do_triangulate()

var _has_triangulation_request: bool = false
## Request that triangulation should be performed on the next frame. 
## Ensures that multiple triangulation requests sent during a single frame will result in just a single triangulation
func triangulate_on_next_frame()->void:
	if only_manual_triangulation: return
	
	 # make sure we don't accidentally triangulate multiple times per frame
	if _has_triangulation_request: return
	_has_triangulation_request = true
	await _level.every_frame_signal
	if not _has_triangulation_request: return
	_has_triangulation_request = false
	do_triangulate()
		
## Immediatelly perform the triangulation
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
		var added :Sector = NodeUtils.instantiate_child(generated_parent, load("res://prefabs/Sector.tscn") as PackedScene) as Sector
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
		 
## Destroy all generated [Sector]s
func do_clear()->void:
	var generated_parent : Node2D = $Generated
	for ch in generated_parent.get_children(): generated_parent.remove_child(ch)

## Make a manually-editable copy of all the generated [Sector]s
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
