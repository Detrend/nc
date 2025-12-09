@tool
class_name WallAttachment
extends Node2D

func find_sector(parent: EditablePolygon = null)->Sector:
	if not parent: 
		parent = get_parent()
		while parent and (parent is not EditablePolygon):
			parent = parent.get_parent()
		if not parent: return null
	
	if parent is Sector: return parent as Sector
	if parent is TriangulatedMultisector:
		var pos := self.global_position
		var multisector := parent as TriangulatedMultisector
		for child in multisector._generated_parent:
			if not child is Sector: continue
			var s : Sector = child as Sector
			if s.contains_point(pos):
				return s
	return null


func find_wall(parent_sector : Sector)->int:
	var pos := self.global_position
	var min_distance_sqr := INF
	var min_idx := -1
	for i in parent_sector.get_walls_count():
		var wall_begin := parent_sector.get_wall_begin(i)
		var wall_end := parent_sector.get_wall_end(i)
		var closest_point := Geometry2D.get_closest_point_to_segment(pos, wall_begin, wall_end)
		var dist_sqr := closest_point.distance_squared_to(pos)
		if dist_sqr < min_distance_sqr:
			min_distance_sqr = dist_sqr
			min_idx = i

	return min_idx


func _get_display_position(parent_sector : Sector, wall_idx : int)->Vector2:
	var wall_begin := parent_sector.get_wall_begin(wall_idx)
	var wall_end := parent_sector.get_wall_end(wall_idx)
	return (wall_begin + wall_end) * 0.5


var _icon : Node2D:
	get: return $Icon
	
func _process(delta: float) -> void:
	pass#_update_visuals(find_sector())

func _on_parent_selected_update(parent_polygon : EditablePolygon, _selected_list: Array[Node])->void:
	_update_visuals(find_sector(parent_polygon))	
	
func _selected_update(_selected_list: Array[Node])->void:
	_update_visuals(find_sector())	

func _update_visuals(parent : Sector)->void:
	var icon := _icon
	if icon:
		var wall := find_wall(parent)
		var display_pos = _get_display_position(parent, wall)
		icon.global_position = display_pos
