extends Object
class_name SectorNeighborhoodAccelerator


var _map : Dictionary[Vector2, Array] = {}


func add_sector(s: Sector)->void:
	for p in s.get_points():
		add_point(p)
	
func add_point(p: SectorPoint)->void:
	var pos := p.global_position
	var bucket_raw = _map.get(pos)
	var bucket : Array[SectorPoint]
	if bucket_raw == null:
		bucket = []
		_map[pos] = bucket
	else:
		bucket = bucket_raw
	bucket.append(p)
	
func get_neighboring_points(global_pos: Vector2)->Array[SectorPoint]:
	return _map.get(global_pos)
