@tool
class_name Level
extends Node2D

var config : EditorConfig = preload("res://config.tres")
@export var coloring_mode: EditorConfig.SectorColoringMode = EditorConfig.SectorColoringMode.Floor
@export var max_snapping_distance : float = 1.0
@export_tool_button("Snap points") var snap_points_tool_button = _snap_points


@export var export_scale : Vector3 = Vector3(1.0, 1.0, 1.0)
@export var export_path : String = ""
@export_tool_button("Export level") var export_level_tool_button = export_level

func _ready() -> void:
	if ! Engine.is_editor_hint(): export_level()

func _process(delta: float) -> void:
	#if ! Engine.is_editor_hint(): return
	_handle_new_sector_creation()

func create_level_export_data() -> Dictionary:	
	var level_export := Dictionary()
	var points_export : Array[PackedFloat32Array]
	var points_map : Dictionary[Vector2, int] = {}
	var sectors_map : Dictionary[Sector, int] = {}
	var get_point_idx : Callable = func(vec: Vector2)->int:
		var ret = points_map.get(vec)
		if ret == null:
			ret = points_export.size()
			var v := _get_export_coords(vec)
			var point_export : PackedFloat32Array = [v.x, v.y]
			points_export.append(point_export)
			points_map[vec] = ret
		return ret as int
		
	var get_destination_portal_wall_idx: Callable = func(s: Sector, destination : int)->int:
		return destination
		return s.get_points_count() - 2 - destination
		var destination_portal_points := s.portal_destination.get_points()
		var first_idx :int = get_point_idx.call(destination_portal_points[0].global_position)
		var this_idx: int = get_point_idx.call(destination_portal_points[destination].global_position)
		return this_idx - first_idx
		
	var all_sectors := get_sectors();
	
	for s in all_sectors: s.sanity_check()
	
	for t in range(all_sectors.size()):
		sectors_map[all_sectors[t]] = t
	
	var host_portals : Dictionary[Sector, Array] = {}
	for s in all_sectors:
		if s.has_portal() and s.is_portal_bidirectional:
			host_portals.get_or_add(s.portal_destination, []).append(s)
	
	var sectors_export : Array[Dictionary]
	for sector in all_sectors:
		var sector_export := Dictionary()
		var sector_points : PackedInt32Array = []
		for p in sector.get_points():
			var coords := p.global_position
			var coords_idx : int = get_point_idx.call(coords)
			sector_points.append(coords_idx)
			
		sector_export["debug_name"] = sector.name
		sector_export["id"] = sectors_map[sector]
		sector_export["floor"] = sector.floor_height * export_scale.z
		sector_export["ceiling"] = sector.ceiling_height * export_scale.z
		sector_export["points"] = sector_points
		sector_export["portal_target"] = sectors_map[sector.portal_destination] if sector.has_portal() else 65535
		sector_export["portal_wall"] = sector.portal_wall if sector.has_portal() else -1
		sector_export["portal_destination_wall"] = get_destination_portal_wall_idx.call(sector.portal_destination, sector.portal_destination_wall) if sector.has_portal() else 0
		var hosts = host_portals.get(sector)
		if hosts != null and ! hosts.is_empty():
			if sector.has_portal(): ErrorUtils.report_error("Sector {0} already has its own portal, but also has host portal from {1}".format([sector, hosts]))
			elif hosts.size() > 1: ErrorUtils.report_error("Sector {0} has {1} host portals - currently unsupported!".format([sector, hosts.size()]))
			var host :Sector = hosts[0]
			sector_export["portal_target"] = sectors_map[host]
			sector_export["portal_wall"] = host.portal_destination_wall
			sector_export["portal_destination_wall"] = host.portal_wall
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
	print("export completed")
	

func get_sectors() -> Array[Sector]:
	var ret : Array[Sector] = []
	ret = NodeUtils.get_children_by_predicate(self, func(ch:Node)->bool: return is_instance_of(ch, Sector) and ch.visible , ret, NodeUtils.LOOKUP_FLAGS.RECURSIVE)
	return ret


func _get_export_coords(p : Vector2)-> Vector2:
	p -= _get_player_position()
	return Vector2(-p.x * export_scale.x, -p.y * export_scale.y)

func _get_player_position() -> Vector2:
	var ret : PlayerPosition = NodeUtils.get_descendant_of_type(self, PlayerPosition)
	if ret: return ret.global_position
	return Vector2.ZERO

func _snap_points()->void:
	for s in get_sectors():
		for p in s.get_points():
			var to_snap = find_nearest_point(p.global_position, max_snapping_distance, s)
			if to_snap and (p.global_position != to_snap.global_position):
				print("snap {0} -> {1} (distance: {2})".format([p, to_snap, p.global_position.distance_to(to_snap.global_position)]))
				p.global_position = to_snap.global_position
	

func find_nearest_point(pos: Vector2, max_snapping_distance: float, to_skip: Sector)->SectorPoint:
	var other_points : Array[SectorPoint] = []
	for other_sector in get_sectors():
		if other_sector == to_skip: continue
		other_points.append_array(other_sector.get_points())
	#print("finding nearest among {0} points".format([other_points.size()]))
	var ret: SectorPoint= Sector.find_nearest_point(pos, other_points, max_snapping_distance)
	return ret

var sector_creation_request : bool = false
func _handle_new_sector_creation()->void:
	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):# and Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
		if (!sector_creation_request) and Input.is_physical_key_pressed(KEY_SHIFT) and Input.is_physical_key_pressed(KEY_CTRL):
			sector_creation_request = true
			var new_sector := add_sector(get_global_mouse_position(), [Vector2(0, 0), Vector2(10, 0), Vector2(0, 10)])
			print("new sector: {0}".format([new_sector]))
	else:
		sector_creation_request = false

func find_sector_parent(position: Vector2)->Node2D:
	var sector_parent : Node2D = null
	if sector_parent == null:
		print("according to selected")
		var selection := EditorInterface.get_selection().get_selected_nodes()
		for s in selection:
			if s is not Sector: continue
			if sector_parent == null or sector_parent == s.get_parent():
				sector_parent = s.get_parent()
			else:
				sector_parent = null
				break
	if sector_parent == null:
		var nearest_point := find_nearest_point(position, INF, null)
		sector_parent = nearest_point._sector.get_parent() if (nearest_point != null) else null
		print("nearest point: {0}".format([nearest_point]))
	if sector_parent == null: sector_parent = $Sectors
	if sector_parent == null: sector_parent = self
	return sector_parent


func add_sector(position: Vector2, points: PackedVector2Array, name: String = "")->Sector:
			var nearest_point := find_nearest_point(position, INF, null)
			var sector_parent : Node2D = find_sector_parent(position)
			
			var new_sector :Sector = preload("res://prefabs/Sector.tscn").instantiate()
			sector_parent.add_child(new_sector)
			new_sector.owner = get_tree().edited_scene_root
			new_sector.global_position = position
			new_sector.polygon = points
			if ! name.is_empty(): new_sector.name = name
			return new_sector
