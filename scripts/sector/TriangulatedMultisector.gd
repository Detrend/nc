@tool
class_name TriangulatedMultisector
extends EditablePolygon

## Perform the triangulation (Alt+T)
@export_tool_button("Triangulate") var triangulate_tool_button = do_triangulate

## Do debug stuff
@export var debug : bool = false

@export var data : SectorProperties = SectorProperties.new()


#func _selected_update(_selected_list: Array[Node])->void:
#	super._selected_update(_selected_list)
#	if _selected_list.find(self) == -1:
#		triangulate_on_next_frame()

func on_editing_start()->void:
	super.on_editing_start()
	do_clear()

func on_editing_finish(_start_was_called_first : bool)->void:
	super.on_editing_finish(_start_was_called_first)
	triangulate_on_next_frame()


func on_descendant_editing_start(_ancestor: EditablePolygon)->void:
	super.on_descendant_editing_start(_ancestor)
	do_clear()

func on_descendant_editing_finish(_ancestor: EditablePolygon, _start_was_called_first: bool):
	super.on_descendant_editing_finish(_ancestor, _start_was_called_first)
	triangulate_on_next_frame()

func _ready() -> void:
	do_triangulate()

var has_triangulation_request: bool = false
func triangulate_on_next_frame()->void:
	if has_triangulation_request: return
	has_triangulation_request = true
	await _level.every_frame_signal
	if not has_triangulation_request: return
	has_triangulation_request = false
	do_triangulate()
		

func do_triangulate()->void:
	do_clear()
	#print("triangulating!")
	var generated_parent : Node2D = $Generated
	var hole_nodes : Array[EditablePolygon] = []
	hole_nodes = NodeUtils.get_children_of_type(self, Sector, hole_nodes)
	var holes : Array[PackedVector2Array] = []
	for hole_node in hole_nodes:
		if hole_node.is_visible_in_tree() and hole_node.is_editable:
			holes.append(hole_node.get_point_positions())
	var computed_segments := GeometryUtils.polygon_to_convex_segments(self.get_point_positions(), holes, debug)
	var counter : int = 0
	for segment in computed_segments:
		#print("clockwise: {0}".format([Geometry2D.is_polygon_clockwise(segment)]))
		var added :Sector = NodeUtils.instantiate_child(generated_parent, preload("res://prefabs/Sector.tscn")) as Sector
		added.global_position = Vector2.ZERO
		added.polygon = segment
		added.is_editable = false
		added.data = self.data
		added.z_index = -10
		added.name = "_" + str(counter)
		counter += 1
	await _level.every_frame_signal	
	for child in generated_parent.get_children():
		(child as Sector)._update_visuals() 	
	
		 
func do_clear()->void:
	var generated_parent : Node2D = $Generated
	for ch in generated_parent.get_children(): generated_parent.remove_child(ch)
