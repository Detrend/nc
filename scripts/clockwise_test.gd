extends Node2D

func _process(delta: float) -> void:
	var is_clockwise : int = GeometryUtils.is_clockwise_triangle($A.global_position, $B.global_position, $C.global_position)
	print("clockwise: {0}".format([is_clockwise]))
