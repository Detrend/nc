@tool
class_name Sector
extends Polygon2D

@export var floor_height : float = 0.0
@export var ceiling_height : float = 10.0

@export var portal_destination : Sector = null
@export var portal_wall : int:
	get: return portal_wall
	set(value): portal_wall = value if polygon.size() <= 0 else (value % polygon.size())
@export var portal_destination_wall: int:
	get: return portal_destination_wall
	set(value): portal_destination_wall = value if !(portal_destination and portal_destination.polygon.size()>0) else (value % portal_destination.polygon.size())
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
	var polygon : PackedVector2Array = []
	_visualizer_line.clear_points()
	for point in get_points():
		_visualizer_line.add_point(point.global_position - _visualizer_line.global_position)
	
	_visualize_portals()
	#if Input.is_physical_key_pressed(KEY_S):
	#	print("snapping...")


func get_points() -> Array[SectorPoint]:
	var ret : Array[SectorPoint] = []
	for i in range(self.polygon.size()):
		ret.append(SectorPoint.new(self, i))
	return ret
		

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

func snap_to_nearest() -> void:
	var level := _level
	var max_snapping_distance_sqr := level.max_snapping_distance * level.max_snapping_distance
	
	var snapping_point := find_nearest_point(get_global_mouse_position(), get_points(), 5)
	
	var nearest_point : SectorPoint = null
	var nearest_point_distance_sqr : float = INF
	for sector in level.get_sectors():
		if sector == self: continue
		var p := find_nearest_point(snapping_point.global_position, sector.get_points(), level.max_snapping_distance)
		var distance_sqr := p.global_position.distance_squared_to(snapping_point.global_position)
		if distance_sqr < nearest_point_distance_sqr:
			nearest_point = p
			nearest_point_distance_sqr = distance_sqr
	if nearest_point:
		self.global_position = nearest_point.global_position
		print("snapped {0} to {1}".format([self, nearest_point]))
	else:
		print("cannot snap {0}".format([self]))


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
