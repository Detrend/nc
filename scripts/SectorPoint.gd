@tool
class_name SectorPoint
extends Node2D

var _sector : Sector:
	get: return NodeUtils.get_ancestor_of_type(self, Sector)

var _level : Level:
	get: return NodeUtils.get_ancestor_of_type(self, Level)

func _input(event: InputEvent) -> void:
	if event.is_action_pressed("Snap"):
		snap_to_nearest()

@export var is_wall : bool = false
@export_tool_button("Snap") var snap_tool_buttonn = snap_to_nearest

func snap_to_nearest() -> void:
	var this_sector := _sector
	var level := _level
	var max_snapping_distance_sqr := level.max_snapping_distance * level.max_snapping_distance
	
	var nearest_point : SectorPoint = null
	var nearest_point_distance_sqr : float = INF
	for sector in level.get_sectors():
		if sector == this_sector: continue
		
		for p in sector.get_points():
			var distance_sqr := p.global_position.distance_squared_to(self.global_position)
			if distance_sqr >= max_snapping_distance_sqr: continue
			if distance_sqr < nearest_point_distance_sqr:
				nearest_point = p
				nearest_point_distance_sqr = distance_sqr
	if nearest_point:
		self.global_position = nearest_point.global_position
		print("snapped {0} to {1}".format([self, nearest_point]))
	else:
		print("cannot snap {0}".format([self]))
