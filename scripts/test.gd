@tool
extends Node

@export var a_begin : Vector2 = Vector2.ZERO
@export var a_end : Vector2 = Vector2.ZERO

@export var b_begin : Vector2 = Vector2.ZERO
@export var b_end : Vector2 = Vector2.ZERO

@export var perform:bool = false:
	get: return false
	set(val):
		do_perform()
		

func _process(delta: float)->void:
	pass
	#var intersect :Variant= Geometry2D.get_closest_point_to_segment(b_end, a_begin, a_end)
	#print("intersect: [{0}]".format([intersect]))

func do_perform()->void:
	var intersect = Geometry2D.merge_polygons($A.polygon, $B.polygon)
	for p in intersect:
		GeometryUtils.ensure_points_are_preserved(p, $A.polygon)
		GeometryUtils.ensure_points_are_preserved(p, $B.polygon)
	for ch in $Results.get_children(): ch.queue_free()
	for i in intersect:
		var new_child : Polygon2D = $C.duplicate()
		$Results.add_child(new_child)
		new_child.owner = self.owner
		new_child.show()
		new_child.polygon = i		
