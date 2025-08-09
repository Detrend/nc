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
@export var export_offset : Vector2 = Vector2.ZERO
@export var export_path : String = ""
@export_tool_button("Export level") var export_level_tool_button = export_level


static var SECTOR_PREFAB : Resource = preload("res://prefabs/Sector.tscn")
static var TRIANGULATED_MULTISECTOR_PREFAB : Resource:
	get: return load("res://prefabs/TriangulatedMultisector.tscn")
static var STAIR_MAKER_PREFAB : Resource = preload("res://prefabs/StairMaker.tscn")


var _editor_plugin : EditorPlugin:
	get:
		if !_editor_plugin: _editor_plugin = EditorPlugin.new()
		return _editor_plugin

func get_undo_redo()->EditorUndoRedoManager: 
	return _editor_plugin.get_undo_redo()
func get_undo_redo_raw()->UndoRedo: 
	var unre := get_undo_redo()
	var ret := unre.get_history_undo_redo(unre.get_object_history_id(self))
	if not ret: print("Level::get_undo_redo_raw()... no unre: {0}".format([unre.get_object_history_id(self)]))
	return ret

signal every_frame_signal()


func _process(delta: float) -> void:
	#if ! Engine.is_editor_hint(): return
	_handle_selections()
	_handle_input()
	_handle_new_sector_creation()
	every_frame_signal.emit()


func _find_connection(connections: Array, c: Callable)->int:
	var i:int = 0
	while i < connections.size():
		var other: Callable = connections[i]["callback"]
		if other.get_method() == c.get_method() and other.get_object() == c.get_object():
			return i
		i += 1
	return -1
	
func _ready() -> void:
	if ! Engine.is_editor_hint(): 
		export_level()
		return
	config.on_cosmetics_changed.connect(_update_sector_visuals)
	await self.every_frame_signal
	await self.every_frame_signal
	var to_register : Callable = _update_sector_visuals
	var unre := get_undo_redo_raw()
	if unre and not unre.version_changed.is_connected(_update_sector_visuals):
		unre.version_changed.connect(_update_sector_visuals, )
		
func _exit_tree() -> void:
	config.on_cosmetics_changed.disconnect(_update_sector_visuals)
	var unre := get_undo_redo_raw()
	if unre and unre.version_changed.is_connected(_update_sector_visuals):
		unre.version_changed.disconnect(_update_sector_visuals)

func _update_sector_visuals()->void:
	for s in get_editable_polygons():
		if s.is_editable: s._update_visuals()

class WallExportData:
	# Material with the highest priority so far
	var material : SectorMaterial
	# Full export-enabled sectors participating in the wall, expected to have 1 or 2 elements
	var sectors : Array[Sector] = []
	# Number of holes participating in the wall, can be arbitrary number
	var holes_count : int = 0

	func register_sector(s: Sector)->void:
		if not s.exclude_from_export:
			sectors.append(s)
		else: holes_count += 1
		if self.material == null || self.material.wall_priority < s.data.material.wall_priority:
			self.material = s.data.material

	func compute_wall_height(this_sector : Sector)->float:
		if sectors.size() == 1:
			return sectors[0].ceiling_height - sectors[0].floor_height
		if sectors.size() != 2:
			ErrorUtils.report_error("Wall has invalid number of sectors: [{0}]".format([TextUtils.recursive_array_tostring(sectors,", ", func(s: Sector): return s.get_full_name() )]))
			if sectors.size() == 0: return 0
		var other_sector = sectors[1 if sectors[0] == this_sector else 0]
		var floor_delta :float = other_sector.floor_height - this_sector.floor_height
		var ceiling_delta :float = this_sector.ceiling_height - other_sector.ceiling_height
		return max(floor_delta, ceiling_delta)

	func export_wall_data(this_sector : Sector)->Dictionary:
		var height := compute_wall_height(this_sector)
		var chosen_material :SectorMaterial = material if (material.wall_priority > this_sector.data.material.wall_priority) else this_sector.data.material
		for rule in chosen_material.wall_rules:
			var height_check := (rule.wall_length_range.x < 0) or (rule.wall_length_range[0] <= height and height <= rule.wall_length_range[1])
			var surface_type_check := ((rule._placement_type & WallRule.PlacementType.Any == WallRule.PlacementType.Any)
									   or ((rule._placement_type & WallRule.PlacementType.Border) and sectors.size() == 2)
									   or ((rule._placement_type & WallRule.PlacementType.HoleBorder) and sectors.size() >= 1 and holes_count >= 1)
									   or ((rule._placement_type & WallRule.PlacementType.Wall) and sectors.size() == 1)
			)
			if height_check and surface_type_check:
				return Level.get_texture_config(rule, 'wall', this_sector)

		return Level.get_texture_config(chosen_material, 'wall', this_sector)


static func _get_wall_idx(wall_begin: Vector2, wall_end: Vector2)->Vector4:
	if wall_begin < wall_end: return Vector4(wall_begin.x, wall_begin.y, wall_end.x, wall_end.y)
	else: return Vector4(wall_end.x, wall_end.y, wall_begin.x, wall_begin.y)

func create_level_export_data() -> Dictionary:	
	var level_export := Dictionary()
	var points_export : Array[PackedFloat32Array]
	var points_map : Dictionary[Vector2, int] = {}
	var sectors_map : Dictionary[Sector, int] = {}
	var walls_map : Dictionary[Vector4, WallExportData] = {}
	var get_point_idx : Callable = func(vec: Vector2)->int:
		var ret = points_map.get(vec)
		if ret == null:
			ret = points_export.size()
			var v := _get_export_coords(vec)
			var point_export : PackedFloat32Array = [v.x, v.y]
			points_export.append(point_export)
			points_map[vec] = ret
		return ret as int
	var get_wall_data : Callable = func(a: Vector2, b: Vector2)->WallExportData:
		var idx:= _get_wall_idx(a, b)
		var ret = walls_map.get(idx)
		if ret == null:
			ret = WallExportData.new()
			walls_map[idx] = ret
		return ret as WallExportData

	var all_sectors := get_sectors(true);
	var all_pickups := get_pickups()
	var all_entities := get_entities()

	var pickups_export : Array[Dictionary] = []
	var entities_export : Array[Dictionary] = []

	
	Sector.sanity_check_all(self, all_sectors)
	self.level_sanity_checks()
	
	for t in range(all_sectors.size()):
		sectors_map[all_sectors[t]] = t
	
	var host_portals : Dictionary[Sector, Array] = {}
	for s in all_sectors:
		if s.has_portal() and s.is_portal_bidirectional:
			host_portals.get_or_add(s.portal_destination, []).append(s)
	
	for s in get_sectors(false):
		var i:= 0
		var walls_count := s.get_walls_count()
		while i < walls_count:
			var wall_data :WallExportData = get_wall_data.call(s.get_wall_begin(i), s.get_wall_end(i))
			wall_data.register_sector(s)
			i += 1
	
	var sectors_export : Array[Dictionary]
	for sector in all_sectors:
		var sector_export := Dictionary()
		var sector_points : PackedInt32Array = []
		for coords in sector.get_point_positions():
			var coords_idx : int = get_point_idx.call(coords)
			sector_points.append(coords_idx)

		process_things_in_sector(all_pickups, sector, pickups_export)
		process_things_in_sector(all_entities, sector, entities_export)

		sector_export["debug_name"] = sector.name
		sector_export["id"] = sectors_map[sector]
		sector_export["floor"] = sector.data.floor_height * export_scale.z
		sector_export["ceiling"] = sector.data.ceiling_height * export_scale.z
		sector_export["points"] = sector_points

		sector_export["floor_surface"] = get_texture_config(sector.data.material, 'floor', sector)
		sector_export["ceiling_surface"] = get_texture_config(sector.data.material, 'ceiling', sector)
		var walls_export : Array[Dictionary] = []
		var wall_idx := 0
		var walls_count = sector.get_walls_count()
		while wall_idx < walls_count:
			var wall_data : WallExportData = get_wall_data.call(sector.get_wall_begin(wall_idx), sector.get_wall_end(wall_idx))
			walls_export.append(wall_data.export_wall_data(sector))
			wall_idx += 1
		sector_export["wall_surfaces"] = walls_export

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
	level_export["pickups"] = pickups_export
	level_export["entities"] = entities_export

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
	
static func get_texture_config(sector_material: Variant, texture_name: String, sector : Sector)->Dictionary:
	if sector_material == null: sector_material = (load("res://textures/default_texture.tres") as SectorMaterial)
	var ret : Dictionary = {}
	ret["id"] = sector_material.get(texture_name + "_texture")
	ret["scale"] = sector_material.get(texture_name + "_scale")
	ret["show"] = sector_material.get("show_" + texture_name)
	var base_rotation_deg :float = sector_material.get(texture_name + "_rotation")
	var custom_rotation_deg :float = sector.data.wall_texturing_rotation if (texture_name == 'wall') else sector.data.texturing_rotation
	ret["rotation"] = deg_to_rad(base_rotation_deg + custom_rotation_deg)
	var offset := sector.data.wall_texturing_offset if (texture_name == 'wall') else sector.data.texturing_offset
	ret["offset"] = [offset.x, offset.y]
	return ret


func get_sectors(for_export_only:bool = true) -> Array[Sector]:
	var flags := NodeUtils.LOOKUP_FLAGS.RECURSIVE | NodeUtils.LOOKUP_FLAGS.INCLUDE_INTERNAL
	var ret : Array[Sector] = []
	ret = NodeUtils.get_children_by_predicate(self, func(n:Node)->bool: return n is Sector and (n as Node2D).is_visible_in_tree() and (!for_export_only or !n.exclude_from_export) , ret, flags)
	return ret

func get_pickups(pickup_type = PickUp, ret: Array[PickUp] = [])->Array[PickUp]:
	ret = NodeUtils.get_children_by_predicate(self, func(n:Node)->bool: return is_instance_of(n, pickup_type) and (n as Node2D).is_visible_in_tree(), ret, NodeUtils.LOOKUP_FLAGS.RECURSIVE)
	return ret

func get_entities(entity_type = Entity, ret: Array[Entity] = [])->Array[Entity]:
	ret = NodeUtils.get_children_by_predicate(self, func(n:Node)->bool: return is_instance_of(n, entity_type) and (n as Node2D).is_visible_in_tree(), ret, NodeUtils.LOOKUP_FLAGS.RECURSIVE)
	return ret

func get_editable_polygons() -> Array[EditablePolygon]:
	var ret : Array[EditablePolygon] = []
	ret = NodeUtils.get_children_by_predicate(self, func(n:Node)->bool: return n is EditablePolygon and n.is_visible_in_tree and n.is_editable, ret, NodeUtils.LOOKUP_FLAGS.RECURSIVE)
	return ret

func process_things_in_sector(all_things: Array, sector: Sector, export_things: Array[Dictionary])->void:
	var i:= 0
	while i < all_things.size():
		var current :Thing = all_things[i]
		var pos := current.global_position
		if sector.contains_point(pos):
			var current_export : Dictionary = {}
			var export_coords_2d = _get_export_coords(pos)
			var height :float
			if current.placement_mode == Things.PlacementMode.Floor: height = sector.floor_height + current.height_offset
			elif current.placement_mode == Things.PlacementMode.Ceiling: height = sector.floor_height - current.height_offset
			elif current.placement_mode == Things.PlacementMode.Absolute: height = current.height_offset
			else: ErrorUtils.report_error("Invalid placement_mode '{0}' for thing {1}".format([current.placement_mode, current]))
			current_export['position'] = [export_coords_2d.x, export_coords_2d.y, height * export_scale.z]
			current.custom_export(sector, current_export)
			export_things.append(current_export)
			all_things.remove_at(i)
		else:
			i += 1

func _get_export_coords(p : Vector2)-> Vector2:
	p += self.export_offset
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
	var ret: SectorPoint = Sector.find_nearest_point(pos, other_points, max_snapping_distance)
	return ret


func level_sanity_checks()->void:
	var all_players := get_entities(PlayerPosition)
	if all_players.size() != 1: ErrorUtils.report_error("Invalid number of Player Positions: {0}".format([all_players.size()]))


#region INPUT_MANAGEMENT

func _handle_new_sector_creation()->void:
	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):# and Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
		if is_key_pressed(KEY_SHIFT) and is_key_pressed(KEY_CTRL) and is_mouse_down(MOUSE_BUTTON_LEFT):
			var prefab_to_use := TRIANGULATED_MULTISECTOR_PREFAB if is_key_pressed(KEY_R) else SECTOR_PREFAB
			var command := make_add_sector_command(prefab_to_use, get_global_mouse_position(), [Vector2(0, 0), Vector2(10, 0), Vector2(0, 10)])
			if false:
				add_sector(command)
			else:
				var unre := get_undo_redo()
				unre.create_action("Create " + "Sector" if prefab_to_use == SECTOR_PREFAB else "Multisector")
				add_sector(command, unre)
				unre.commit_action()


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
	return resource_name + "-" + str(randi_range(0, 9999))

class AddSectorCommand:
	var parent : Node
	var child_idx : int = -1
	var prefab : Resource
	var name: String
	var position: Vector2
	var points: PackedVector2Array
	var data: SectorProperties

func make_add_sector_command(prefab: Resource, position: Vector2, points: PackedVector2Array, name: String = "", parent: Node2D = null)->AddSectorCommand:
	var ret := AddSectorCommand.new()
	var nearest_point := find_nearest_point(position, INF, null)
	ret.parent = (parent if parent else find_sector_parent(position, nearest_point))
	ret.prefab = prefab
	ret.name = name if not name.is_empty() else generate_random_name(prefab)
	ret.position = position
	ret.points = points
	ret.data = nearest_point._sector.data.duplicate() if (self.auto_heights and nearest_point) else null
	return ret

func add_sector(command: AddSectorCommand, unre: EditorUndoRedoManager = null)->EditablePolygon:	
	var parent :Node = command.parent
	var new_sector :EditablePolygon 
	if unre == null:
		new_sector = NodeUtils.instantiate_child(parent, command.prefab) as EditablePolygon
		if command.child_idx >= 0:
			parent.move_child(new_sector, command.child_idx)
	else:
		new_sector = command.prefab.instantiate() as EditablePolygon
		NodeUtils.add_do_undo_child(unre, parent, new_sector, command.child_idx)
	new_sector.name = command.name
	unre.add_do_property(new_sector, 'global_position', command.position)
	new_sector.global_position = command.position
	new_sector.polygon = command.points
	if command.data != null:
		new_sector.data = command.data
	return new_sector





enum KeypressState{
	NONE = 0, UP, DOWN, PRESSED
}
static var KEYS_TO_TRACK : Array[Key] = [KEY_ALT, KEY_SHIFT, KEY_CTRL, KEY_Q, KEY_P, KEY_L, KEY_T, KEY_R, KEY_X]
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
var current_selection : Array[Node]
func _handle_selections()->void:
	if last_selection == null: last_selection = []
	var current_selection : Array[Node] = []
	NodeUtils.get_selected_nodes_of_type(Node, current_selection)
	self.current_selection = current_selection
	if last_selection.size() == 1 and ( current_selection.size() != 1 or current_selection[0] != last_selection[0]) and last_selection[0] and last_selection[0] is EditablePolygon:
		(last_selection[0] as EditablePolygon)._on_sole_unselected()
	if current_selection.size() == 1 and ( last_selection.size() != 1 or last_selection[0] != current_selection[0]) and current_selection[0] and current_selection[0] is EditablePolygon:
		(current_selection[0] as EditablePolygon)._on_sole_selected()
	for last in last_selection:
		if last and last is EditablePolygon and current_selection.find(last) < 0:
			(last as EditablePolygon)._selected_update(current_selection)
	for current in current_selection:
		if current and current is EditablePolygon:
			(current as EditablePolygon)._selected_update(current_selection)
	last_selection = current_selection
	

#endregion
