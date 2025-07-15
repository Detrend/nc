@tool
class_name Sector
extends Polygon2D

@export var floor_height : float = 0.0
@export var ceiling_height : float = 1.5

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

var config : EditorConfig = preload("res://config.tres")

func has_portal()-> bool: return enable_portal and portal_destination != null

var _level : Level:
	get: return NodeUtils.get_ancestor_component_of_type(self, Level)

var _visualizer_line : Line2D:
	get: return $VisualizerLine


var last_snapping_frame : int = -1 
var last_points : PackedVector2Array = []

func _process(delta: float) -> void:
	if ! Engine.is_editor_hint(): return
	
	self.scale = Vector2.ONE
	self.rotation = 0.0
	var points := self.polygon
	self._handle_alt_mode_snapping(points)
	
	if !is_just_being_edited() :
		var did_change :bool = false
		did_change = did_change or self._remove_duplicit_points (points)
		did_change = did_change or self._ensure_points_clockwise(points)
		if did_change:
			print("before: {0}".format([points]))
			self.polygon = points
			print("after:  {0}".format([self.polygon]))
		
	
	_visualize_border()
	_visualize_portals()
	
	self.color = EditorConfig.get_sector_color(config, self)
	last_points = points


func get_points() -> Array[SectorPoint]:
	var ret : Array[SectorPoint] = []
	for i in range(self.polygon.size()):
		ret.append(SectorPoint.new(self, i))
	return ret

func get_points_count() -> int:
	return self.polygon.size() 
	
func point_pos_relative_to_absolute(point: Vector2)-> Vector2:
	return point + self.global_position
func point_pos_absolute_to_relative(point: Vector2)->Vector2:
	return point - self.global_position
	
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



var is_target_of_alt_mode_snapping : bool = false
var affected_points : Array[SectorPoint] = []
var selected_idx : int = -1
var last_sector_pos : Vector2 = Vector2.ZERO

func _alt_mode_snap(current_points: PackedVector2Array)->int:
	var last_pos := self.last_sector_pos
	self.last_sector_pos = self.global_position
	if last_sector_pos != self.global_position: return -1 # reset if the whole sector was moved
	if ! Input.is_physical_key_pressed(KEY_Q): return -1 # reset if Q is not pressed
	if current_points.size() != last_points.size(): return -1 # reset if point count changed
	if ! NodeUtils.is_the_only_selected_node(self): return -1 # reset if this is not the single selected node
	
	var max_distance_sqr = config.shared_continuous_movement_max_step * config.shared_continuous_movement_max_step
	var t: int = 0
	var selected :int = -1
	while t < current_points.size():
		if current_points[t] != last_points[t]:
			if selected != -1: return -1 # reset if it's more than one point that changed
			selected = t
		t += 1
	if selected == -1 and is_just_being_edited():
		selected = self.selected_idx
		
	var current_absolute := point_pos_relative_to_absolute(current_points[selected])
	if selected != self.selected_idx:
		#print("!!!!! {0} > {1}".format([selected, self.selected_idx]))
		self.selected_idx = selected
		for p in self.affected_points: p._sector.is_target_of_alt_mode_snapping = false
		self.affected_points.clear()
		var previous_absolute := point_pos_relative_to_absolute(last_points[selected])
		for s in _level.get_sectors():
			if s == self: continue
			for p in s.get_points():
				if p.global_position == previous_absolute and (p.global_position.distance_squared_to(current_absolute) < max_distance_sqr):
					p._sector.is_target_of_alt_mode_snapping = true
					self.affected_points.append(p)
	for p in self.affected_points: 
		p.global_position = current_absolute

	return selected

func _handle_alt_mode_snapping(points: PackedVector2Array)->bool:
	self.selected_idx = _alt_mode_snap(points)
	if self.selected_idx == -1:
		for p in self.affected_points: p._sector.is_target_of_alt_mode_snapping = false
		affected_points.clear()
	if self.selected_idx != -1: 
		print("{1}: {0}".format([self.selected_idx, name]))
	return self.selected_idx != -1



func sanity_check()->void:
	if self.polygon.size() < 3:
		ErrorUtils.report_error("{0} - only {1} vertices".format([get_full_name(), self.polygon.size()]))
		return

func is_just_being_edited()->bool:
	return	( is_target_of_alt_mode_snapping 
				or (Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT) && NodeUtils.is_the_only_selected_node(self))
			)

static func find_nearest_point(v: Vector2, others: Array[SectorPoint], tolerance: float)-> SectorPoint:
	var toleranceSqr : float = tolerance *tolerance
	var ret : SectorPoint = null
	var ret_distance : float = INF
	for p in others:
		var distance : float = p.global_position.distance_squared_to(v)
		if distance >= toleranceSqr: continue
		if distance < ret_distance:
			print("({4}) better: {0} < {1} ({2} < {3})".format([distance, ret_distance, p, ret, v]))
			ret = p
			ret_distance = distance
	return ret	

func get_full_name()->String:
	var ret : Array[String] = []
	var node : Node= self
	while (node != null) and (node is not Level):
		ret.append(node.name)
		node = node.get_parent()
	ret.reverse()
	return DatastructUtils.string_concat(ret, "::")
