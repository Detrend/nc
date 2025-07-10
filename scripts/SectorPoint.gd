class_name SectorPoint

var _sector : Sector
var _idx : int

func _init(sector : Sector, idx : int) -> void:
	_sector = sector
	_idx = idx

var global_position : Vector2:
	get: return _sector.point_pos_relative_to_absolute(_sector.polygon[_idx])
	set(value): 
		var points := _sector.polygon
		points[_idx] = _sector.point_pos_absolute_to_relative(value)
		_sector.polygon = points

func _to_string():
	return "{0}[{1}]".format([_sector.name, _idx])
