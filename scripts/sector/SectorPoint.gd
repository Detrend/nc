class_name SectorPoint

var _sector: EditablePolygon
var _idx : int

func _init(sector : EditablePolygon, idx : int) -> void:
	_sector = sector
	_idx = idx

var global_position : Vector2:
	get: return _sector.get_point_position(_idx)
	set(value): 
		_sector.set_point_position(_idx, value)

func _to_string():
	return "{0}[{1}]".format([_sector.get_full_name(), _idx])

func _equals(other: SectorPoint)->bool:
	return (_sector == other._sector) and (_idx == other._idx)
