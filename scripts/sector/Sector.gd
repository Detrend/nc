@tool
class_name Sector
extends EditablePolygon

@export var data : SectorProperties = SectorProperties.new()
@export var floor_height : float:
	get: return data.floor_height
	set(val): 
		data.floor_height = val
		_update_visuals()
		
@export var ceiling_height : float:
	get: return data.ceiling_height
	set(val): 
		data.ceiling_height = val
		_update_visuals()


@export var exclude_from_export : bool = false:
	get: return exclude_from_export
	set(val): 
		exclude_from_export = val
		_update_visuals()


@export_group("Portal")

@export var portal_destination : Sector = null:
	get: return portal_destination
	set(value):
		portal_destination = value
		_update_visuals()
		
@export var enable_portal : bool = true:
	get: return enable_portal
	set(value):
		enable_portal = value
		_update_visuals()
		
@export var portal_wall : int:
	get: return portal_wall
	set(value): 
		portal_wall = value if polygon.size() <= 0 else (value % polygon.size())
		_update_visuals()
		
@export var portal_destination_wall: int:
	get: return portal_destination_wall
	set(value): 
		portal_destination_wall = value if !(portal_destination and portal_destination.get_points_count()>0) else (value % portal_destination.get_points_count())
		_update_visuals()
		
@export var is_portal_bidirectional : bool = true:
	get: return is_portal_bidirectional
	set(value):
		is_portal_bidirectional = value
		_update_visuals()
		
@export var show_portal_arrow : bool = false:
	get: return show_portal_arrow
	set(value):
		show_portal_arrow = value
		_update_visuals()

func has_portal()-> bool: return enable_portal and portal_destination != null and portal_destination.is_visible_in_tree()


var _visualizer_line : Line2D:
	get: return get_node_or_null("VisualizerLine")

func get_own_prefab()->Resource:
	return Level.SECTOR_PREFAB

# to make sure update_visuals() isn't called from some setter at the very beginning before config and scene is even set up 
var _did_start : bool = false
func _ready() -> void:
	_did_start = true
	_update_visuals()

func _update_visuals() -> void:
	if not _did_start: return
	_visualize_border()
	_visualize_portals()
	self.color = EditorConfig.get_sector_color(config, self)


func on_editing_start()->void:
	super.on_editing_start()
	_update_visuals()

func on_editing_finish(_start_was_called_first : bool)->void:
	super.on_editing_finish(_start_was_called_first)
	_update_visuals()

func _selected_update(_selected_list: Array[Node])->void:
	super._selected_update(_selected_list)
	if is_editable:
		_update_visuals()

func _alt_mode_drag_update()->void:
	super._alt_mode_drag_update()
	_update_visuals()

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
	if $PortalDestinationVisualizerLine: $PortalDestinationVisualizerLine.clear_points()
	$PortalArrowLine.clear_points()
	if ! has_portal(): 
		return
	var these_points := get_points()
	$PortalVisualizerLine.add_point(these_points[portal_wall].global_position - self.global_position)
	$PortalVisualizerLine.add_point(these_points[(portal_wall + 1)%these_points.size()].global_position  - self.global_position)
	var target_points := portal_destination.get_points()
	if $PortalDestinationVisualizerLine: 
		$PortalDestinationVisualizerLine.add_point(target_points[portal_destination_wall].global_position - self.global_position)
		$PortalDestinationVisualizerLine.add_point(target_points[(portal_destination_wall + 1) % target_points.size()].global_position - self.global_position)
	if show_portal_arrow:
		$PortalArrowLine.add_point((these_points[portal_wall].global_position + these_points[(portal_wall + 1)%these_points.size()].global_position)*0.5  - self.global_position)
		$PortalArrowLine.add_point((target_points[portal_destination_wall].global_position + target_points[(portal_destination_wall + 1)%target_points.size()].global_position)*0.5  - self.global_position)
	
	$PortalVisualizerLine.default_color = config.portal_entry_color
	if $PortalDestinationVisualizerLine: $PortalDestinationVisualizerLine.default_color = config.portal_exit_color_bidirectional if self.is_portal_bidirectional else config.portal_exit_color_single



func _remove_duplicit_points(points: PackedVector2Array)->bool:
	var did_remove : bool = false
	var previous : PackedVector2Array = []
	previous.append_array(points)
	var t:int = 1
	while t < points.size():
		if points[t] == points[t-1]:
			print("removing idx: {0} : {1} (same as {2}) - size: {3}".format([t, points[t], points[t-1], points.size()]))
			points.remove_at(t-1)
			did_remove = true
		else: t += 1
	if points[points.size() - 1] == points[0]:
		print("removing idx: {0} : {1} (same as {2}) - size: {3}".format([points.size() - 1, points[points.size() - 1], points[0], points.size()]))
		points.remove_at(points.size() - 1)
		did_remove = true
	return did_remove

func _ensure_points_clockwise(points: PackedVector2Array)->bool:
	if Geometry2D.is_polygon_clockwise(points):
		print("reverting {0}".format([get_full_name()]))
		points.reverse()
		return true
	return false



#region SANITY_CHECKS

static func sanity_check_all(level: Level, all_sectors: Array[Sector])->void:
	print("\tCheck - trivial")
	for s in all_sectors:
		if s.get_points_count() < 3:
			ErrorUtils.report_error("{0} - only {1} vertices".format([s.get_full_name(), s.get_points_count()]))
	for s in all_sectors:
		if s.ceiling_height <= s.floor_height:
			ErrorUtils.report_error("{0} - ceiling <= floor ({1} <= {2})".format([s.get_full_name(), s.ceiling_height, s.floor_height])) 
			
	if level.config.sanity_check_convex:
		print("\tCheck - convexity...")
		for s in all_sectors:
			if not s.exclude_from_export:
				if not s.is_convex():
					ErrorUtils.report_error("Non-convex: {0}".format([s.get_full_name()]))
		
	if level.config.sanity_check_intersecting:
		print("\tCheck - intersections...")
		var i: int = 0
		while i < all_sectors.size():
			var sector_a := all_sectors[i].get_point_positions()
			var j: int = 0
			while j < i:
				var intersection := Geometry2D.intersect_polygons(sector_a, all_sectors[j].get_point_positions())
				if intersection.size() > 0:
					ErrorUtils.report_error("{0} intersects {1}".format([all_sectors[i].get_full_name(), all_sectors[j].get_full_name()]))
				j += 1
			i += 1
	
	if level.config.sanity_check_snapping:
		print("\tCheck - unsnapped...")
		var snapping_distance_sqr := level.config.max_snapping_distance * level.config.max_snapping_distance
		var i: int = 0
		while i < all_sectors.size():
			var sector_a := all_sectors[i].get_points()
			var j: int = 0
			while j < i:
				var sector_b := all_sectors[j].get_points()
				for a in sector_a:
					for b in sector_b:
						var distance_sqr : float = a.global_position.distance_squared_to(b.global_position)
						if 0 < distance_sqr and distance_sqr <= snapping_distance_sqr:
							ErrorUtils.report_warning("Unsnapped points: {0} <-> {1}".format([a, b]))
				j += 1
			i += 1
	


func is_convex()->bool:
	return Geometry2D.decompose_polygon_in_convex(self.polygon).size() == 1
	
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

#region GETTERS

static func get_sectors(this: Node, ret : Array[Sector] = [], include_uneditable: bool = false, lookup_flags: NodeUtils.LOOKUP_FLAGS = NodeUtils.LOOKUP_FLAGS.RECURSIVE):
	ret = NodeUtils.get_children_by_predicate(this, func(n:Node)->bool: return n is Sector and (include_uneditable || n.is_editable), ret, lookup_flags)
	return ret

#endregion
