@tool
class_name TriangulatedMultisector
extends EditablePolygon

## Perform the triangulation (Alt+T)
@export_tool_button("Triangulate") var triangulate_tool_button = do_triangulate

## Do debug stuff
@export var debug : bool = false

@export var sector_data : SectorProperties = SectorProperties.new()

func _ready() -> void:
	do_triangulate()

func do_triangulate()->void:
	var generated_parent : Node2D = $Generated
	for ch in generated_parent.get_children(): generated_parent.remove_child(ch)
	var hole_nodes : Array[EditablePolygon] = []
	hole_nodes = NodeUtils.get_children_of_type(self, Sector, hole_nodes)
	var holes : Array[PackedVector2Array] = []
	for hole_node in hole_nodes:
		if hole_node.is_visible_in_tree() and hole_node.is_editable:
			holes.append(hole_node.get_point_positions())
	var computed_segments := GeometryUtils.polygon_to_convex_segments(self.get_point_positions(), holes, debug)
	for segment in computed_segments:
		#print("clockwise: {0}".format([Geometry2D.is_polygon_clockwise(segment)]))
		var added :Sector = NodeUtils.instantiate_child(generated_parent, preload("res://prefabs/Sector.tscn")) as Sector
		added.global_position = Vector2.ZERO
		added.polygon = segment
		added.is_editable = false
		added.data = self.sector_data
		added.z_index = -10
		 
