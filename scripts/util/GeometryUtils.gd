class_name GeometryUtils


static func mult_members3(a: Vector3, b: Vector3)->Vector3:
	return Vector3(a.x*b.x, a.y*b.y, a.z*b.z)

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

static func distance_sqr_from_line_segment(line_begin: Vector2, line_end: Vector2, point: Vector2)->float:
	if line_begin == line_end: 
		return line_begin.distance_squared_to(point)

	var line_length = line_end.distance_to(line_begin)
	var line_direction :Vector2 = (line_end - line_begin) * (1/line_length)
	var point_rebased := point - line_begin
	var projection_t := line_direction.dot(point_rebased)
	if projection_t < 0:
		return point.distance_squared_to(line_begin)
	if projection_t >= 1.0:
		return point.distance_squared_to(line_end)
	var projection := line_begin + (projection_t * line_direction)
	return projection.distance_squared_to(point)
	

static func distance_from_line_segment(line_begin: Vector2, line_end : Vector2, point: Vector2)->float:
	return sqrt(distance_sqr_from_line_segment(line_begin, line_end, point))


static func find_closest_point(v: Vector2, points: PackedVector2Array, blacklist: Dictionary[Vector2, bool] = {})->int:
	var best_distance_sqr := INF
	var best_idx := -1
	var i := 0
	while i < points.size():
		if blacklist.is_empty() or !blacklist.get(points[i], false):
			var distance_sqr := v.distance_squared_to(points[i])
			if distance_sqr < best_distance_sqr:
				best_distance_sqr = distance_sqr
				best_idx = i
		i+= 1
	return best_idx

static func find_closest_points(a: PackedVector2Array, b: PackedVector2Array, blacklist: Dictionary[Vector2, bool] = {})->Vector2i:
	var ret : Vector2i = Vector2i(-1, -1)
	var best_distance_sqr := INF
	var i := 0
	while i < a.size():
		var closest_idx := find_closest_point(a[i], b, blacklist)
		var distance_sqr := a[i].distance_squared_to(b[closest_idx])
		if distance_sqr < best_distance_sqr:
			best_distance_sqr = distance_sqr
			ret = Vector2i(i, closest_idx)
		i+= 1
	return ret

static func polygon_is_hole(polygon: PackedVector2Array)->bool:
	return Geometry2D.is_polygon_clockwise(polygon) 

static func delta_equals(a: Vector2, b: Vector2)->bool:
	return a.distance_squared_to(b) < 0.01

static func ensure_points_are_preserved(new_polygon: PackedVector2Array, old_polygon: PackedVector2Array)->void:
	var i: int = 0
	while i < new_polygon.size():
		var line_begin := new_polygon[i]
		var line_end := new_polygon[i + 1 if i < new_polygon.size() - 1 else 0]
		var did_find : bool = false
		for old_point in old_polygon:
			var closest := Geometry2D.get_closest_point_to_segment(old_point, line_begin, line_end)
			if delta_equals(closest, old_point) and (closest != line_begin and closest != line_end):
				new_polygon.insert(i + 1, closest)
				did_find = true
				break
		if !did_find:
			i += 1
	

static func merge_polygons(polygons: Array[PackedVector2Array], out_outer : Array[PackedVector2Array], out_holes: Array[PackedVector2Array])->void:
	out_outer.clear()
	out_holes.clear()
	if polygons.is_empty(): return
	
	var outer_temp : PackedInt32Array = []
	var holes_temp : PackedInt32Array = []

	out_outer.append_array(polygons)
	
	var iterations_count : int = 0
	var did_merge : bool = true
	while did_merge and iterations_count < 10:
		iterations_count += 1
		did_merge = false
		var i:int = 0
		while i < out_outer.size():
			var j:int = i + 1
			while j < out_outer.size():
				if j == i: 
					j += 1
					continue
				var merge := Geometry2D.merge_polygons(out_outer[i], out_outer[j])
				if merge.size() == 2 and ! polygon_is_hole(merge[0]) and ! polygon_is_hole(merge[1]):
					#merge returned the original polygons				
					j += 1
					continue
				did_merge = true
				outer_temp.clear()
				holes_temp.clear()
				var merge_idx :int = 0
				while merge_idx < merge.size():
					if polygon_is_hole(merge[merge_idx]):
						holes_temp.append(merge_idx)
					else:
						outer_temp.append(merge_idx)
					merge_idx += 1
				#print("merge[{0}, {1}]: {2} polygons, {3} holes".format([i, j, outer_temp.size(), holes_temp.size()]))
				if outer_temp.size() == 0:
					ErrorUtils.report_warning("Merge produced 0 non-hole polygons (and {0} holes)".format([holes_temp.size()]))
				if outer_temp.size() >= 2:
					ErrorUtils.report_warning("merge produced more than 1 non-hole polygon ({0} in total + {1} holes)".format([outer_temp.size(), holes_temp.size()]))
				
				ensure_points_are_preserved(merge[outer_temp[0]], out_outer[i]) 
				ensure_points_are_preserved(merge[outer_temp[0]], out_outer[j]) 
				for hole_idx in holes_temp: 
					ensure_points_are_preserved(merge[hole_idx], out_outer[i]) 
					ensure_points_are_preserved(merge[hole_idx], out_outer[j]) 
					out_holes.append(merge[hole_idx])
				out_outer[i] = merge[outer_temp[0]]
				out_outer.remove_at(j)
				break

			i += 1


static func polygon_to_convex_segments(polygon: PackedVector2Array, holes: Array[PackedVector2Array], debug: bool = false, aggressive: bool = false, should_merge_holes : bool = true)->Array[PackedVector2Array]:
	
	var holes_merged : Array[PackedVector2Array] = []
	var hole_holes : Array[PackedVector2Array] = []
	if should_merge_holes:
		merge_polygons(holes, holes_merged, hole_holes)
		holes = holes_merged

	var segments : Array[PackedVector2Array] = [polygon]
	for hole in holes:
		var new_segments : Array[PackedVector2Array] = []
		for current_segment in segments:
			var divided := Geometry2D.clip_polygons(current_segment, hole)
			for d in divided:
				ensure_points_are_preserved(d, current_segment)
				ensure_points_are_preserved(d, hole)
			if debug: 
				print("division: {0}".format([divided.size()]))
				for g in divided: print("\tcl: {0}".format([Geometry2D.is_polygon_clockwise(g)]))
			var blacklist : Dictionary[Vector2, bool] = {}
			if divided.size() >= 2 and !polygon_is_hole(divided[0]) and divided.slice(1).all(polygon_is_hole):
				# the function returned just the original polygon and some holes
				var main := divided[0]
				for hole_in_appropriate_direction in divided.slice(1):
					var closest_point_indices := find_closest_points(hole_in_appropriate_direction, main, blacklist)
					var loop :PackedVector2Array = hole_in_appropriate_direction.slice(closest_point_indices.x) + hole_in_appropriate_direction.slice(0, closest_point_indices.x)
					var connection_point := main[closest_point_indices.y]
					loop.append(loop[0])
					loop.append(connection_point)
					blacklist[connection_point] = true
					blacklist[loop[0]] = true

					main = main.slice(0, closest_point_indices.y + 1) + loop + main.slice(closest_point_indices.y + 1)
				new_segments.append(main)
				if aggressive:
					var decomposition := Geometry2D.decompose_polygon_in_convex(main)
					new_segments.append_array(decomposition)
			else:
				new_segments.append_array(divided)
		segments = new_segments

	#segments.append_array(hole_holes)

	if debug: return segments
	
	var ret : Array[PackedVector2Array] = []
	for segment in segments:
		var decomposition := Geometry2D.decompose_polygon_in_convex(segment)
		ret.append_array(decomposition)
	return ret

static func is_convex_polygon(polygon: PackedVector2Array)->bool:
	return Geometry2D.decompose_polygon_in_convex(polygon).size() <= 1

static func get_orthogonal(v: Vector2)->Vector2:
	return Vector2(v.y, -v.x)

static func extrude_along_normals(point_stripe: PackedVector2Array, extrusion_factor : float = 0.2)->PackedVector2Array:
	if point_stripe.size() <= 1:
		ErrorUtils.report_error("Point stripe too small (only {0} points)".format([point_stripe.size()]))
		return point_stripe

	var end :int = point_stripe.size()
	var last_normal := get_orthogonal(point_stripe[end - 1] - point_stripe[end - 2])

	var t: int = end - 1
	while t >= 1:
		var current_normal := get_orthogonal(point_stripe[t] - point_stripe[t - 1])
		var interpolated_normal := (last_normal + current_normal)*0.5
		point_stripe.append(point_stripe[t] + interpolated_normal * extrusion_factor)
		last_normal = current_normal
		t -= 1	
	point_stripe.append(point_stripe[0] + get_orthogonal(point_stripe[1] - point_stripe[0])*extrusion_factor)

	return point_stripe
