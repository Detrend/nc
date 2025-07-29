@tool
class_name Level
extends Node2D

var config : EditorConfig = preload("res://config.tres") as EditorConfig
@export var coloring_mode: EditorConfig.SectorColoringMode = EditorConfig.SectorColoringMode.Floor
var max_snapping_distance : float:
	get: return config.max_snapping_distance
@export_tool_button("Snap points") var snap_points_tool_button = _snap_points
@export var auto_heights:bool = true

@export var export_scale : Vector3 = Vector3(1.0, 1.0, 1.0)
@export var export_path : String = ""
@export_tool_button("Export level") var export_level_tool_button = export_level

func _ready() -> void:
	if ! Engine.is_editor_hint(): export_level()

func _process(delta: float) -> void:
	#if ! Engine.is_editor_hint(): return
	_handle_input()
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
	ret = NodeUtils.get_children_by_predicate(self, func(ch:Node)->bool: return is_instance_of(ch, Sector) and ch.is_visible_in_tree() , ret, NodeUtils.LOOKUP_FLAGS.RECURSIVE)
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
		s._snap_points()
	

func find_nearest_point(pos: Vector2, max_snapping_distance: float, to_skip: Sector)->SectorPoint:
	var other_points : Array[SectorPoint] = []
	for other_sector in get_sectors():
		if other_sector == to_skip: continue
		other_points.append_array(other_sector.get_points())
	#print("finding nearest among {0} points".format([other_points.size()]))
	var ret: SectorPoint= Sector.find_nearest_point(pos, other_points, max_snapping_distance)
	return ret

func _handle_new_sector_creation()->void:
	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):# and Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
		if is_mouse_down(MOUSE_BUTTON_LEFT)  and is_key_pressed(KEY_SHIFT) and is_key_pressed(KEY_CTRL):
			var new_sector := add_sector(get_global_mouse_position(), [Vector2(0, 0), Vector2(10, 0), Vector2(0, 10)])
			print("new sector: {0}".format([new_sector]))
			

func find_sector_parent(position: Vector2, nearest_point : SectorPoint)->Node2D:
	var sector_parent : Node2D = null
	if sector_parent == null:
		print("according to selected")
		var selection := EditorInterface.get_selection().get_selected_nodes()
		for s in selection:
			var current : Node2D = s.get_parent() if s is Sector else s
			if sector_parent == null or sector_parent == current:
				sector_parent = current
			else:
				sector_parent = null
				break
	if sector_parent == null:
		#var nearest_point := find_nearest_point(position, INF, null)
		if nearest_point != null:
			sector_parent = nearest_point._sector.get_parent() if (nearest_point != null) else null
			print("nearest point: {0}".format([nearest_point]))
	if sector_parent == null: sector_parent = $Sectors
	if sector_parent == null: sector_parent = self
	return sector_parent


func add_sector(position: Vector2, points: PackedVector2Array, name: String = "")->Sector:
			var nearest_point := find_nearest_point(position, INF, null)
			var sector_parent : Node2D = find_sector_parent(position, nearest_point)
			
			var new_sector :Sector = NodeUtils.instantiate_child(sector_parent, preload("res://prefabs/Sector.tscn")) as Sector
			new_sector.global_position = position
			new_sector.polygon = points
			if ! name.is_empty(): 
				new_sector.name = name
			if self.auto_heights and nearest_point:
				new_sector.floor_height = nearest_point._sector.floor_height
				new_sector.ceiling_height = nearest_point._sector.ceiling_height
			return new_sector


enum KeypressState{
	NONE = 0, UP, DOWN, PRESSED
}
static var KEYS_TO_TRACK : Array[Key] = [KEY_ALT, KEY_SHIFT, KEY_CTRL, KEY_Q, KEY_P, KEY_L]
static var MOUSE_BUTTONS_TO_TRACK : Array[MouseButton] = [MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT]

var keypress_states : Dictionary[int, KeypressState]

func _update_key_state(key:int, is_keyboard_button : bool)->bool:
	if keypress_states == null: 
		keypress_states = {}
	var last_state : KeypressState = keypress_states.get(key, KeypressState.NONE)
	var last : bool = (last_state != KeypressState.NONE) and (last_state != KeypressState.UP)
	var current : bool = Input.is_key_pressed(key as Key) if is_keyboard_button else Input.is_mouse_button_pressed(key as MouseButton)
	if current != last: 
		keypress_states[key] = KeypressState.DOWN if current else KeypressState.UP
		return true
	else:
		keypress_states[key] = KeypressState.PRESSED if current else KeypressState.NONE
		return false

func is_key_down(key: Key)->bool:
	return keypress_states.get(key, KeypressState.NONE) == KeypressState.DOWN
func is_key_up(key: Key)->bool:
	return keypress_states.get(key, KeypressState.NONE) == KeypressState.UP
func is_key_pressed(key: Key)->bool:
	return Input.is_key_pressed(key)

func is_mouse_down(key: MouseButton)->bool:
	return keypress_states.get(key, KeypressState.NONE) == KeypressState.DOWN
func is_mouse_up(key: MouseButton)->bool:
	return keypress_states.get(key, KeypressState.NONE) == KeypressState.UP
func is_mouse_pressed(key: MouseButton)->bool:
	return Input.is_mouse_button_pressed(key)


func _change_sector_height(node: Sector, delta: float, mode: EditorConfig.SectorColoringMode)->void:
	if   mode == EditorConfig.SectorColoringMode.Floor:
		node.floor_height += delta * config.floor_height_increment
	elif mode == EditorConfig.SectorColoringMode.Ceiling:
		node.ceiling_height += delta * config.ceiling_height_increment


func _handle_input()->void:
	# update general input
	var did_change: bool = false
	for key in KEYS_TO_TRACK:  if _update_key_state(key, true): did_change = true
	for key in MOUSE_BUTTONS_TO_TRACK:  if _update_key_state(key, false): did_change = true
	if did_change:
		#print("Did change: {0}".format([keypress_states]))
		var selected :Array[Sector] = []
		selected = NodeUtils.get_selected_nodes_of_type(Sector, selected)
		if selected.size() == 1: 
			selected[0]._on_sole_selected_input()


	# handle Level-specific input actions
	var increment := 0
	var is_alt_pressed = is_key_pressed(KEY_ALT)
	if is_alt_pressed and is_key_down(KEY_P): 
		print("+")
		increment += 1
	if is_alt_pressed and is_key_down(KEY_L): 
		print("-")
		increment -= 1
	if increment != 0.0:
		for sector in NodeUtils.get_selected_nodes_of_type(Sector):
			_change_sector_height(sector, increment, coloring_mode)
