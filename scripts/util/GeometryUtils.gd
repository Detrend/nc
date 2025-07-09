class_name GeometryUtils


static func look_at_rotation_rad(direction: Vector2)->float:
	return atan2(direction.x, direction.y)

static func multiply_memberwise2(a: Vector2, b: Vector2)->Vector2:
	return Vector2(a.x*b.x, a.y*b.y)
