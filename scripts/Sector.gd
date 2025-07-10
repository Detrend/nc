@tool
class_name Sector
extends Polygon2D

@export var floor_height : float = 0.0
@export var ceiling_height : float = 1.5

@export var portal_destination : Sector = null
@export var portal_wall : int:
	get: return portal_wall
	set(value): portal_wall = value if polygon.size() <= 0 else (value % polygon.size())
@export var portal_destination_wall: int:
	get: return portal_destination_wall
	set(value): portal_destination_wall = value if !(portal_destination and portal_destination.get_points_count()>0) else (value % portal_destination.get_points_count())
@export var is_portal_bidirectional : bool = true
@export var show_portal_arrow : bool = false

var config : EditorConfig = preload("res://config.tres")

func has_portal()-> bool: return portal_destination != null

var _level : Level:
	get: return NodeUtils.get_ancestor_component_of_type(self, Level)

var _visualizer_line : Line2D:
	get: return $VisualizerLine


func _process(delta: float) -> void:
	if ! Engine.is_editor_hint(): return
	
	self.scale = Vector2.ONE
	self.rotation = 0.0
	var points := self.polygon
	self._handle_alt_mode_snapping(points)
	var did_change :bool = false
	did_change = did_change or self._remove_duplicit_points (points)
	did_change = did_change or self._ensure_points_clockwise(points)
	if did_change:
		self.polygon = points
		
	
	_visualize_border()
	_visualize_portals()
	
	#self.color
	#self.color = config.get_floor_color(floor_height)
	self.color = EditorConfig.get_floor_color(config, floor_height)
	#if Input.is_physical_key_pressed(KEY_S):
	#	print("snapping...")


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
	if ! portal_destination: 
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
	var did_remove : bool = false
	var t:int = 1
	while t < points.size():
		if points[t] == points[t-1]:
			points.remove_at(t)
			did_remove = true
		else: t += 1
	return did_remove

func _ensure_points_clockwise(points: PackedVector2Array)->bool:
	if GeometryUtils.is_clockwise_convex_polygon(points) < 0:
		points.reverse()
		return true
	return false


var last_points : PackedVector2Array = []
func _handle_alt_mode_snapping(points: PackedVector2Array)->void:
	if Input.is_physical_key_pressed(KEY_Q):
		var selection := EditorInterface.get_selection().get_selected_nodes()
		if selection.size() == 1 and selection[0] == self:
			if points.size() == last_points.size():
				var t :int = 0
				while t < points.size():
					if points[t] != last_points[t]:
						var previous_absolute := point_pos_relative_to_absolute(last_points[t])
						var current_absolute := point_pos_relative_to_absolute(points[t])
						for s in _level.get_sectors():
							if s == self: continue
							for p in s.get_points():
								if p.global_position == previous_absolute:
									p.global_position = current_absolute
						pass
					t += 1
			#print("Alt mode: {0}".format([self]))
	last_points = points


static func find_nearest_point(v: Vector2, others: Array[SectorPoint], tolerance: float)-> SectorPoint:
	var toleranceSqr : float = tolerance *tolerance
	var ret : SectorPoint = null
	var ret_distance : float = INF
	for p in others:
		var distance : float = p.global_position.distance_squared_to(v)
		if distance >= toleranceSqr: continue
		if distance < ret_distance:
			ret = p
			ret_distance = distance
	return ret	
