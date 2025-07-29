@tool
class_name Sector
extends EditablePolygon

@export var floor_height : float = 0.0
@export var ceiling_height : float = 1.5
@export var exclude_from_export : bool = false

@export_group("Portal")
@export var portal_destination : Sector = null
@export var enable_portal : bool = true
@export var portal_wall : int:
	get: return portal_wall
	set(value): portal_wall = value if polygon.size() <= 0 else (value % polygon.size())
@export var portal_destination_wall: int:
	get: return portal_destination_wall
	set(value): portal_destination_wall = value if !(portal_destination and portal_destination.get_points_count()>0) else (value % portal_destination.get_points_count())
@export var is_portal_bidirectional : bool = true
@export var show_portal_arrow : bool = false

func has_portal()-> bool: return enable_portal and portal_destination != null and portal_destination.is_visible_in_tree()


var _visualizer_line : Line2D:
	get: return $VisualizerLine



func _process(_delta: float) -> void:
	super._process(_delta)

	if ! Engine.is_editor_hint(): return
	if ! self.is_visible_in_tree(): return

	_visualize_border()
	_visualize_portals()
	
	self.color = EditorConfig.get_sector_color(config, self)


func do_postprocess(points: PackedVector2Array)->bool:
	var did_change :bool = false
	did_change = did_change or self._remove_duplicit_points (points)
	did_change = did_change or self._ensure_points_clockwise(points)
	return did_change

func _visualize_border()->void:
	_visualizer_line.clear_points()
	for point in get_points():
		_visualizer_line.add_point(point.global_position - _visualizer_line.global_position)

func _visualize_portals()->void:
	$PortalVisualizerLine.clear_points()
	$PortalDestinationVisualizerLine.clear_points()
	$PortalArrowLine.clear_points()
	if ! has_portal(): 
		return
	var these_points := get_points()
	$PortalVisualizerLine.add_point(these_points[portal_wall].global_position - self.global_position)
	$PortalVisualizerLine.add_point(these_points[(portal_wall + 1)%these_points.size()].global_position  - self.global_position)
	var target_points := portal_destination.get_points()
	$PortalDestinationVisualizerLine.add_point(target_points[portal_destination_wall].global_position - self.global_position)
	$PortalDestinationVisualizerLine.add_point(target_points[(portal_destination_wall + 1) % target_points.size()].global_position - self.global_position)
	if show_portal_arrow:
		$PortalArrowLine.add_point((these_points[portal_wall].global_position + these_points[(portal_wall + 1)%these_points.size()].global_position)*0.5  - self.global_position)
		$PortalArrowLine.add_point((target_points[portal_destination_wall].global_position + target_points[(portal_destination_wall + 1)%target_points.size()].global_position)*0.5  - self.global_position)
	
	$PortalVisualizerLine.default_color = config.portal_entry_color
	$PortalDestinationVisualizerLine.default_color = config.portal_exit_color_bidirectional if self.is_portal_bidirectional else config.portal_exit_color_single



func _remove_duplicit_points(points: PackedVector2Array)->bool:
	#if ! NodeUtils.is_the_only_selected_node(self): return false
	#if points != last_points: return false
	
	var did_remove : bool = false
	var previous : PackedVector2Array = []
	previous.append_array(points)
	var t:int = 1
	while t < points.size():
		if points[t] == points[t-1]:
			print("removing idx: {0} : {1} (same as {2}) - size: {3}".format([t, points[t], points[t-1], points.size()]))
			
			#print("{0}".format([previous]))
			#print("{0}".format([points]))
			
			points.remove_at(t-1)
			#t += 1
			did_remove = true
		else: t += 1
	if did_remove: 
		var temp = [points[points.size()-1]]
		temp.append_array(points.slice(0, points.size()-1))
		points = temp
		print("{0}".format([previous]))
		print("{0}".format([points]))
		print("---------[{0}]".format([Engine.get_frames_drawn()]))
	return did_remove

func _ensure_points_clockwise(points: PackedVector2Array)->bool:
	#if NodeUtils.is_the_only_selected_node(self): return false
	if GeometryUtils.is_clockwise_convex_polygon(points) < 0:
		points.reverse()
		return true
	return false



#region SANITY_CHECKS

func sanity_check()->void:
	if self.polygon.size() < 3:
		ErrorUtils.report_error("{0} - only {1} vertices".format([get_full_name(), self.polygon.size()]))
	_check_is_convex()
	_check_unsnapped_points()
	_check_intersecting_sectors()

func _check_unsnapped_points()->void:
	if not config.sanity_check_snapping: return
	var unsnapped = null
	for this_point in get_points():
		var distance_sqr := _check_is_unsnapped_point(this_point)
		if distance_sqr > 0.0:
			if unsnapped == null: unsnapped = []
			unsnapped.append("{0}({1})".format([this_point._idx, sqrt(distance_sqr)]))
	if unsnapped:
		ErrorUtils.report_warning("Unsnapped: {0} [{1}]".format([get_full_name(), DatastructUtils.string_concat(unsnapped)]))

func _check_intersecting_sectors()->void:
	if not config.sanity_check_intersecting: return
	for this_point in get_points():
		for s in _level.get_sectors():
			if s.contains_point(this_point.global_position, false):
				ErrorUtils.report_warning("{0} intersects {1}".format([this_point, s.get_full_name()]))
			

func _check_is_unsnapped_point(this_point: SectorPoint)->float:
	var snapping_distance_sqr := config.max_snapping_distance * config.max_snapping_distance
	for s in _level.get_sectors():
		if s == self: continue
		for other_point in s.get_points():
			var distance_sqr : float = this_point.global_position.distance_squared_to(other_point.global_position)
			if 0 < distance_sqr and distance_sqr <= snapping_distance_sqr:
				return distance_sqr
	return 0.0
	
func _check_is_convex()->void:
	if not config.sanity_check_convex: return
	if not is_convex():
		ErrorUtils.report_error("Non-convex: {0}".format([self.get_full_name()]))

func is_convex()->bool:
	# algorithm copypasted from: https://www.geeksforgeeks.org/dsa/check-if-given-polygon-is-a-convex-polygon-or-not/
	var previous_sign : float = 0
	var points_count := get_points_count()
	var i: int = 1
	while i <= get_walls_count():
		var origin := get_point_position(i-1)
		var a := get_point_position(i % points_count)
		var b := get_point_position((i + 1) % points_count)
		var current_sign := (a-origin).cross(b-origin)
		if current_sign != 0.0:
			if current_sign * previous_sign < 0.0:
				return false
			previous_sign = current_sign
		i += 1
	return true

#endregion				
