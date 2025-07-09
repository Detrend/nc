@tool
class_name Level
extends Node2D

@export var max_snapping_distance : float = 1.0

@export var export_scale : Vector2 = Vector2(1.0, 1.0)
@export var export_path : String = ""
@export_tool_button("Export level") var export_level_tool_button = export_level



func create_level_export_data() -> Dictionary:	
	var level_export := Dictionary()
	var points_export : Array[PackedFloat32Array]
	var points_map : Dictionary[Vector2, int] = {}
	var get_point_idx : Callable = func(vec: Vector2)->int:
		var ret = points_map.get(vec)
		if ! ret:
			ret = points_export.size()
			var v := _get_actual_coords(vec)
			var point_export : PackedFloat32Array = [v.x, v.y]
			points_export.append(point_export)
			points_map[vec] = ret
		return ret as int
	
	var sectors_export : Array[Dictionary]
	for sector in get_sectors():
		var sector_export := Dictionary()
		var sector_points : PackedInt32Array = []
		for p in sector.get_points():
			var coords := p.global_position
			var coords_idx : int = get_point_idx.call(coords)
			sector_points.append(coords_idx)
			
		sector_export["floor"] = sector.floor_height
		sector_export["ceiling"] = sector.ceiling_height
		sector_export["points"] = sector_points
		sectors_export.append(sector_export)
	level_export["points"] = points_export
	level_export["sectors"] = sectors_export
	return level_export

func export_level():
	print("exporting level to: '{0}'".format([export_path]))
	var data := create_level_export_data()
	
	var json := JSON.stringify(data, "\t")
	var file := FileAccess.open(export_path, FileAccess.WRITE)
	if ! file:
		ErrorUtils.report_error("Export failed - couldn't open file {0}".format([export_path]))
		return
	file.store_string(json)
	file.close()
	

func get_sectors() -> Array[Sector]:
	var ret : Array[Sector] = []
	return NodeUtils.get_children_of_type(self, Sector, ret)

func _get_actual_coords(p : Vector2)-> Vector2:
	return Vector2(p.x * export_scale.x, p.y * export_scale.y)
