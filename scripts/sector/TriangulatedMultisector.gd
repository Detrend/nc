@tool
class_name TriangulatedMultisector
extends Polygon2D

@export_tool_button("Triangulate") var triangulate_tool_button = do_triangulate

func do_triangulate()->void:
	print("Triangulating!")
	var generated_parent : Node2D = $Generated
	for ch in generated_parent.get_children(): generated_parent.remove_child(ch)
	var hole_nodes : Array[HoleSector] = []
	hole_nodes = NodeUtils.get_children_of_type(self, HoleSector, hole_nodes)
	var holes : Array[PackedVector2Array] = []
	for hole_node in hole_nodes:
		if hole_node.is_visible_in_tree(): holes.append(hole_node.get_point_positions())
	var computed_segments := GeometryUtils.polygon_to_convex_segments(self.polygon, holes)
	for segment in computed_segments:
		print("clockwise: {0}".format([Geometry2D.is_polygon_clockwise(segment)]))
		var added :Sector = NodeUtils.instantiate_child(generated_parent, preload("res://prefabs/Sector.tscn")) as Sector
		added.global_position = Vector2.ZERO
		added.polygon = segment
		 
