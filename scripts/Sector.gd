@tool
class_name Sector
extends Node2D

@export var floor_height : float = 0.0
@export var ceiling_height : float = 10.0

var _visualizer_poly : Polygon2D:
	get: return $VisualizerPoly

var _visualizer_line : Line2D:
	get: return $VisualizerLine


func _process(delta: float) -> void:
	if ! Engine.is_editor_hint(): return
	
	var polygon : PackedVector2Array = []
	_visualizer_line.clear_points()
	var first_point : SectorPoint = null
	for p_raw in NodeUtils.get_children_of_type(self, SectorPoint):
		var point : SectorPoint = p_raw
		if ! first_point: first_point = point
		polygon.append(point.global_position - _visualizer_poly.global_position)
		_visualizer_line.add_point(point.global_position - _visualizer_line.global_position)
	if first_point: _visualizer_line.add_point(first_point.global_position - _visualizer_line.global_position)
	_visualizer_poly.polygon = polygon
	
	pass

func get_points() -> Array[SectorPoint]:
	var ret : Array[SectorPoint] = []
	return NodeUtils.get_children_of_type(self, SectorPoint, ret)
