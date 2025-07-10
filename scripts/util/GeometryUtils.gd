class_name GeometryUtils


static func look_at_rotation_rad(direction: Vector2)->float:
	return atan2(direction.x, direction.y)

static func multiply_memberwise2(a: Vector2, b: Vector2)->Vector2:
	return Vector2(a.x*b.x, a.y*b.y)

static func is_clockwise_triangle(a: Vector2, b: Vector2, c: Vector2)->int:
	var dir := b - a
	var p := c-a
	var result = dir.cross(p)
	return sign(result)


static func is_clockwise_convex_polygon(a: PackedVector2Array)->int:
	#assumes a must be convex
	var t : int = 2
	while t < a.size():
		var cl := is_clockwise_triangle(a[t-2], a[t-1], a[t])
		if cl != 0: return cl
		t += 1
	return 0	
