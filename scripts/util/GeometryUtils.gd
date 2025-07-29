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

static func line_vs_point(line_origin: Vector2, line_direction: Vector2, point: Vector2)->float:
	return (line_direction).cross(point-line_origin)


static func find_closest_point(v: Vector2, points: PackedVector2Array)->int:
	var best_distance_sqr := INF
	var best_idx := -1
	var i := 0
	while i < points.size():
		var distance_sqr := v.distance_squared_to(points[i])
		if distance_sqr < best_distance_sqr:
			best_distance_sqr = distance_sqr
			best_idx = i
		i+= 1
	return best_idx

static func find_closest_points(a: PackedVector2Array, b: PackedVector2Array)->Vector2i:
	var ret : Vector2i = Vector2i(-1, -1)
	var best_distance_sqr := INF
	var i := 0
	while i < a.size():
		var closest_idx := find_closest_point(a[i], b)
		var distance_sqr := a[i].distance_squared_to(b[closest_idx])
		if distance_sqr < best_distance_sqr:
			best_distance_sqr = distance_sqr
			ret = Vector2i(i, closest_idx)
		i+= 1
	return ret

static func polygon_to_convex_segments(polygon: PackedVector2Array, holes: Array[PackedVector2Array])->Array[PackedVector2Array]:
	var segments : Array[PackedVector2Array] = [polygon]
	for hole in holes:
		var new_segments : Array[PackedVector2Array] = []
		for current_segment in segments:
			var divided := Geometry2D.clip_polygons(current_segment, hole)
			print("division: {0}".format([divided.size()]))
			for g in divided: print("\tcl: {0}".format([Geometry2D.is_polygon_clockwise(g)]))
			if divided.size() >= 2 and !Geometry2D.is_polygon_clockwise(divided[0]) and divided.slice(1).all(Geometry2D.is_polygon_clockwise):
				# the function returned just the original polygon and a hole
				var main := divided[0]
				for hole_in_appropriate_direction in divided.slice(1):
					var closest_point_indices := find_closest_points(hole_in_appropriate_direction, main)
					var loop :PackedVector2Array = hole_in_appropriate_direction.slice(closest_point_indices.x) + hole_in_appropriate_direction.slice(0, closest_point_indices.x)
					var connection_point := main[closest_point_indices.y]
					loop.append(loop[0])
					loop.append(connection_point)

					main = main.slice(0, closest_point_indices.y + 1) + loop + main.slice(closest_point_indices.y + 1)
				new_segments.append(main)
				#var decomposition := Geometry2D.decompose_polygon_in_convex(main)
				#new_segments.append_array(decomposition)
			else:
				new_segments.append_array(divided)
		segments = new_segments

	#return segments
	var ret : Array[PackedVector2Array] = []
	for segment in segments:
		var decomposition := Geometry2D.decompose_polygon_in_convex(segment)
		ret.append_array(decomposition)
	return ret
