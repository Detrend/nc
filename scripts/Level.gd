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


var _editor_plugin : EditorPlugin:
	get:
		if !_editor_plugin: _editor_plugin = EditorPlugin.new()
		return _editor_plugin


signal every_frame_signal()

func _ready() -> void:
	if ! Engine.is_editor_hint(): export_level()

func _process(delta: float) -> void:
	#if ! Engine.is_editor_hint(): return
	_handle_selections()
	_handle_input()
	_handle_new_sector_creation()
	every_frame_signal.emit()

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
		
	var all_sectors := get_sectors(true);
	
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
		sector_export["floor"] = sector.data.floor_height * export_scale.z
		sector_export["ceiling"] = sector.data.ceiling_height * export_scale.z
		sector_export["points"] = sector_points
		sector_export["portal_target"] = sectors_map[sector.portal_destination] if sector.has_portal() else 65535
		sector_export["portal_wall"] = sector.portal_wall if sector.has_portal() else -1
		sector_export["portal_destination_wall"] = sector.portal_destination_wall if sector.has_portal() else 0
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
	

func get_sectors(for_export_only:bool = true) -> Array[Sector]:
	var ret : Array[Sector] = []
	ret = NodeUtils.get_children_by_predicate(self, func(ch:Node)->bool: return ch is Sector and ch.is_visible_in_tree() and (!for_export_only or !ch.exclude_from_export) , ret, NodeUtils.LOOKUP_FLAGS.RECURSIVE)
	return ret


func get_editable_polygons() -> Array[EditablePolygon]:
	var ret : Array[EditablePolygon] = []
	ret = NodeUtils.get_children_by_predicate(self, func(ch:Node)->bool: return ch is EditablePolygon and ch.is_visible_in_tree and ch.is_editable, ret, NodeUtils.LOOKUP_FLAGS.RECURSIVE)
	return ret


func _get_export_coords(p : Vector2)-> Vector2:
	p -= _get_player_position()
	return Vector2(-p.x * export_scale.x, -p.y * export_scale.y)

func _get_player_position() -> Vector2:
	var ret : PlayerPosition = NodeUtils.get_descendant_of_type(self, PlayerPosition)
	if ret: return ret.global_position
	return Vector2.ZERO

func _snap_points()->void:
	for s in get_editable_polygons():
		s._snap_points()
	

func find_nearest_point(pos: Vector2, max_snapping_distance: float, to_skip: Sector)->SectorPoint:
	var other_points : Array[SectorPoint] = []
	for other_sector in get_sectors():
		if !other_sector.is_editable: continue
		if other_sector == to_skip: continue
		other_points.append_array(other_sector.get_points())
	#print("finding nearest among {0} points".format([other_points.size()]))
	var ret: SectorPoint= Sector.find_nearest_point(pos, other_points, max_snapping_distance)
	return ret


func _handle_new_sector_creation()->void:
	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):# and Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
		if is_key_pressed(KEY_SHIFT) and is_key_pressed(KEY_CTRL) and is_mouse_down(MOUSE_BUTTON_LEFT):
			if is_key_pressed(KEY_T):
				var new_multisector := add_sector(preload("res://prefabs/TriangulatedMultisector.tscn"), get_global_mouse_position(), [Vector2(0, 0), Vector2(10, 0), Vector2(0, 10)])
				print("new multisector: {0}".format([new_multisector]))
			else:
				var new_sector := add_sector(preload("res://prefabs/Sector.tscn"), get_global_mouse_position(), [Vector2(0, 0), Vector2(10, 0), Vector2(0, 10)])
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
		if nearest_point != null:
			sector_parent = nearest_point._sector.get_parent() if (nearest_point != null) else null
			print("nearest point: {0}".format([nearest_point]))
	if sector_parent == null: sector_parent = $Sectors
	if sector_parent == null: sector_parent = self
	return sector_parent


func generate_random_name(prefab: Resource)->String:
	var resource_name :String = TextUtils.substring(prefab.resource_path, prefab.resource_path.rfind("/") + 1, prefab.resource_path.rfind(".tscn"))
	print("{0}  |  {1} ({2})".format([prefab.resource_path, resource_name, prefab.resource_path.rfind(".tscn")]))
	return resource_name + "-" + str(randi_range(0, 9999))

func add_sector(prefab: Resource, position: Vector2, points: PackedVector2Array, name: String = "")->EditablePolygon:
			var nearest_point := find_nearest_point(position, INF, null)
			var sector_parent : Node2D = find_sector_parent(position, nearest_point)
			
			var new_sector :EditablePolygon = NodeUtils.instantiate_child(sector_parent, prefab) as EditablePolygon
			new_sector.global_position = position
			new_sector.polygon = points
			new_sector.name = name if not name.is_empty() else generate_random_name(prefab)
			if self.auto_heights and nearest_point:
				new_sector.data = nearest_point._sector.data.duplicate()
			return new_sector


enum KeypressState{
	NONE = 0, UP, DOWN, PRESSED
}
static var KEYS_TO_TRACK : Array[Key] = [KEY_ALT, KEY_SHIFT, KEY_CTRL, KEY_Q, KEY_P, KEY_L, KEY_T]
static var MOUSE_BUTTONS_TO_TRACK : Array[MouseButton] = [MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE]

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


func _change_sector_height(unre: EditorUndoRedoManager, node: Sector, delta: float, mode: EditorConfig.SectorColoringMode)->void:
	node.data = node.data.duplicate()
	if   mode == EditorConfig.SectorColoringMode.Floor:
		unre.add_do_property(node, 'floor_height', node.floor_height + delta * config.floor_height_increment)
		unre.add_undo_property(node, 'floor_height', node.floor_height)
	elif mode == EditorConfig.SectorColoringMode.Ceiling:
		unre.add_do_property(node, 'ceiling_height', node.ceiling_height + delta * config.ceiling_height_increment)
		unre.add_undo_property(node, 'floor_height', node.ceiling_height)


func _handle_input()->void:
	# update general input
	var did_change: bool = false
	for key in KEYS_TO_TRACK:  if _update_key_state(key, true): did_change = true
	for key in MOUSE_BUTTONS_TO_TRACK:  if _update_key_state(key, false): did_change = true
	if not did_change:
		return
	#print("Did change: {0}".format([keypress_states]))
	var selected :Array[Node] = []
	selected = NodeUtils.get_selected_nodes_of_type(Node, selected)
	for node in selected: 
		if node is EditablePolygon:
			node._on_selected_input(selected)

	_handle_stairs_increment()
	_handle_triangulation_command(selected)
			

func _handle_stairs_increment()->void:
	# handle Level-specific input actions
	var increment := 0
	var is_alt_pressed = is_key_pressed(KEY_ALT)
	if is_alt_pressed and is_key_down(KEY_P): 
		increment += 1
	if is_alt_pressed and is_key_down(KEY_L): 
		increment -= 1
	if increment != 0.0:
		var affected_sectors : Array[Sector] = []
		affected_sectors = NodeUtils.get_selected_nodes_of_type(Sector, affected_sectors)
		var unre := _editor_plugin.get_undo_redo()
		unre.create_action("{0} {1} height ({2})".format(["Increment" if increment > 0 else "Decrement", "floor" if coloring_mode == EditorConfig.SectorColoringMode.Floor else "ceiling", affected_sectors.size()]))
		for sector in affected_sectors:
			_change_sector_height(unre, sector, increment, coloring_mode)
		unre.commit_action()

func _handle_triangulation_command(selected: Array[Node])->void:
	var is_alt_pressed = is_key_pressed(KEY_ALT)
	if is_alt_pressed and is_key_down(KEY_T):
		var selected_multisectors : Dictionary[TriangulatedMultisector, bool] = {}
		for sel in selected:
			var multipolygon := NodeUtils.get_ancestor_of_type(sel, TriangulatedMultisector) as TriangulatedMultisector
			if multipolygon != null:
				selected_multisectors[multipolygon] = true
		for sel in selected_multisectors.keys():
			sel.do_triangulate()


var last_selection : Array[Node]
func _handle_selections()->void:
	if last_selection == null: last_selection = []
	var current_selection : Array[Node] = []
	NodeUtils.get_selected_nodes_of_type(Node, current_selection)
	if last_selection.size() == 1 and ( current_selection.size() != 1 or current_selection[0] != last_selection[0]) and last_selection[0] is EditablePolygon:
		(last_selection[0] as EditablePolygon)._on_sole_unselected()
	if current_selection.size() == 1 and ( last_selection.size() != 1 or last_selection[0] != current_selection[0]) and current_selection[0] is EditablePolygon:
		(current_selection[0] as EditablePolygon)._on_sole_selected()
	for last in last_selection:
		if last is EditablePolygon and current_selection.find(last) < 0:
			(last as EditablePolygon)._selected_update(current_selection)
	for current in current_selection:
		if current is EditablePolygon:
			(current as EditablePolygon)._selected_update(current_selection)
	last_selection = current_selection
	
