@tool
extends Polygon2D
class_name EditablePolygon


@export_tool_button("Snap points") var snap_points_tool_button = _snap_points

var is_editable : bool = true

var config : EditorConfig = preload("res://config.tres") as EditorConfig


var _level : Level:
	get: return NodeUtils.get_ancestor_component_of_type(self, Level)


func _selected_update(_selected_list: Array[Node])->void:
	self._manage_points()
	
func _alt_mode_drag_update()->void:
	self._manage_points()

func _on_sole_selected()->void:
	if not is_editable:
		var editable_ancestor : Node = NodeUtils.get_ancestor_by_predicate(self, func(n:Node): return (n is EditablePolygon) and n.is_editable)
		print("Selected a non-editable node (ancestor: {0})".format([editable_ancestor]))
		if editable_ancestor: 
			var selection := EditorInterface.get_selection()
			selection.clear()
			selection.add_node(editable_ancestor)
	pass

func _on_sole_unselected()->void:
	pass

func _on_selected_input(selected_list: Array[Node])->void:
	if selected_list.size() == 1:
		if _level.is_mouse_down(MOUSE_BUTTON_LEFT):
			on_editing_start()
		elif _level.is_mouse_up(MOUSE_BUTTON_LEFT):
			on_editing_finish(true)
		elif _level.is_mouse_up(MOUSE_BUTTON_RIGHT):
			on_editing_finish(false)


func on_editing_start()->void:
	NodeUtils.try_send_message_to_ancestor(self, EditablePolygon, 'on_descendant_editing_start', [self])

func on_editing_finish(_start_was_called_first : bool)->void:
	NodeUtils.try_send_message_to_ancestor(self, EditablePolygon, 'on_descendant_editing_finish', [self, _start_was_called_first])


func on_descendant_editing_start(_ancestor: EditablePolygon)->void:
	NodeUtils.try_send_message_to_ancestor(self, EditablePolygon, 'on_descendant_editing_start', [self])

func on_descendant_editing_finish(_ancestor: EditablePolygon, _start_was_called_first: bool):
	NodeUtils.try_send_message_to_ancestor(self, EditablePolygon, 'on_descendant_editing_finish', [self, _start_was_called_first])


func _on_config_change()->void:
	pass


var last_points : PackedVector2Array = []
	

func _manage_points()->void:
	self.scale = Vector2.ONE
	self.rotation = 0.0
	
	var points := self.polygon
	
	self._handle_alt_mode_snapping(points)
	
	if !is_just_being_edited() :
		var did_change :bool = do_postprocess(points)
		if did_change:
			#print(get_full_name())
			#print("before: {0}".format([points]))
			self.polygon = points
			#print("after:  {0}".format([self.polygon]))
		
	last_points = points
	

func do_postprocess(_points: PackedVector2Array)->bool:
	return false
	

func get_points() -> Array[SectorPoint]:
	var ret : Array[SectorPoint] = []
	for i in range(self.polygon.size()):
		ret.append(SectorPoint.new(self, i))
	return ret

func get_points_count() -> int:
	return self.polygon.size() 
	
func get_point_position(idx: int)->Vector2:
	return point_pos_relative_to_absolute(self.polygon[idx])
	
func set_point_position(idx: int, absolute: Vector2)->void:
	var points := self.polygon
	points[idx] = self.point_pos_absolute_to_relative(absolute)
	self.polygon = points
	
func get_point_positions()->PackedVector2Array:
	return DatastructUtils.modify_in_place(self.polygon, point_pos_relative_to_absolute)
	
func set_point_positions(positions_gets_consumed: PackedVector2Array)->void:
	self.polygon = DatastructUtils.modify_in_place(positions_gets_consumed, point_pos_relative_to_absolute)
	
func get_wall_begin(idx: int)->Vector2:
	return get_point_position(idx)
func get_wall_end(idx: int)->Vector2:
	var point_idx := idx + 1
	if point_idx >= get_points_count(): point_idx = 0
	return get_point_position(point_idx)
func get_walls_count()->int:
	return get_points_count()
	
func point_pos_relative_to_absolute(point: Vector2)-> Vector2:
	return _get_snapped_to_grid(point + self.global_position)
func point_pos_absolute_to_relative(point: Vector2)->Vector2:
	return _get_snapped_to_grid(point) - self.global_position
	
	
func _get_snapped_to_grid(pos: Vector2)->Vector2:
		# compute as much as possible with float vars because floats use 64-bit precision unlike Vector2's x,y fields
		var multiplier_x :float = round(pos.x * config.gridsnap_step_inv)
		var multiplier_y :float = round(pos.y * config.gridsnap_step_inv)
		var snapped : Vector2 = Vector2(multiplier_x * config.gridsnap_step, multiplier_y * config.gridsnap_step)
		return snapped


func _snap_points_to_grid()->void:
	for p in self.get_points():
		p.global_position = _get_snapped_to_grid(p.global_position)


func _snap_points()->void:
	_snap_points_to_grid()
	return
	for p in self.get_points():
		var to_snap = _level.find_nearest_point(p.global_position, _level.max_snapping_distance, self)
		if to_snap and (p.global_position != to_snap.global_position):
			print("snap {0} -> {1} (distance: {2})".format([p, to_snap, p.global_position.distance_to(to_snap.global_position)]))
			p.global_position = to_snap.global_position


func contains_point(p: Vector2, inclusive: bool = true)->bool:
	var sgn :float= sign(GeometryUtils.line_vs_point(get_wall_begin(0), get_wall_end(0) - get_wall_begin(0), p))
	if sgn == 0 and !inclusive: return false
	var i: int = 1
	while i < get_walls_count():
		var current_sign :float= sign(GeometryUtils.line_vs_point(get_wall_begin(i), get_wall_end(i) - get_wall_begin(i), p))
		if current_sign == 0:
			if ! inclusive: return false
		else:
			if sgn == 0:
				sgn == current_sign
			if sgn != current_sign:
				return false
		i += 1
	return true

var _last_alt_mode_snapping_change_framestamp : int = 0

var is_target_of_alt_mode_snapping : bool = false:
	get: return is_target_of_alt_mode_snapping
	set(val):
		if val == is_target_of_alt_mode_snapping: return
		is_target_of_alt_mode_snapping = val
		var current_time := Engine.get_frames_drawn()
		if _last_alt_mode_snapping_change_framestamp < current_time:
			if val: on_editing_start()
			if !val: on_editing_finish(true)
		_last_alt_mode_snapping_change_framestamp = current_time

enum AltModeState{
	NONE = 0, WAITING_FOR_MOVEMENT, ACTIVE
}
func alt_mode_state_is_none(state: AltModeState)->bool:
	return state != AltModeState.WAITING_FOR_MOVEMENT && state != AltModeState.ACTIVE

var alt_mode_state: AltModeState = AltModeState.NONE
var alt_mode_selected_idx : int = -1
var alt_mode_affected_points : Array[SectorPoint]
var alt_mode_saved_configurations : Dictionary[EditablePolygon, PackedVector2Array]

func find_changed_point_index(current_points: PackedVector2Array)->int:
		var t: int = 0
		var selected :int = -1
		while t < current_points.size():
			if current_points[t] != last_points[t]:
				if selected != -1: 
					ErrorUtils.report_error("Alt edit mode: more than one point changed {2}[{0}] as well as [{1}]".format([t, selected, get_full_name()]))
				selected = t
			t += 1
		return selected

func find_points_to_alt_mode_snap(this_point_idx: int)->Array[SectorPoint]:
	var ret : Array[SectorPoint] = []
	var previous_absolute := self.point_pos_relative_to_absolute(last_points[this_point_idx])
	for s in _level.get_editable_polygons():
			for p in s.get_points():
				if p._sector == self and p._idx == this_point_idx: continue
				if p.global_position == previous_absolute:
					ret.append(p)
	return ret

func do_alt_mode_retire(_current_points: PackedVector2Array)->void:
	var unre : EditorUndoRedoManager= _level._editor_plugin.get_undo_redo()

	# first pop the edit-polygon action that was added to history just before this
	unre.get_history_undo_redo(unre.get_object_history_id(self)).undo()
	self.polygon = _current_points # make sure this sector has the new polygon and not the old one resulting from the undo

	unre.create_action("Multimove ({0} points / {1} sectors)".format([alt_mode_affected_points.size() + 1, alt_mode_saved_configurations.size()]))
	for sector in alt_mode_saved_configurations.keys():
		#print("adding redo for {0}: {1}".format([sector.get_full_name(), sector.polygon]))
		unre.add_do_property(sector, 'polygon', sector.polygon)
		unre.add_do_method(sector, 'on_editing_finish', false)
		#print("adding undo for {0}: {1}".format([sector.get_full_name(), alt_mode_saved_configurations[sector]]))
		unre.add_undo_property(sector, 'polygon', alt_mode_saved_configurations[sector])
		unre.add_undo_method(sector, 'on_editing_finish', false)
		sector.is_target_of_alt_mode_snapping = false
	self.is_target_of_alt_mode_snapping = false
	unre.commit_action()
	
	alt_mode_selected_idx = -1
	alt_mode_affected_points.clear()
	alt_mode_saved_configurations.clear()
	pass

func _alt_mode_snap(current_points: PackedVector2Array)->void:
	
	if alt_mode_state_is_none(alt_mode_state):
		if is_just_being_edited() and _level.is_key_pressed(KEY_Q):
			alt_mode_state = AltModeState.WAITING_FOR_MOVEMENT
			# no return
	if alt_mode_state == AltModeState.WAITING_FOR_MOVEMENT:
		if ! is_just_being_edited():
			alt_mode_state = AltModeState.NONE
			return

		self.alt_mode_selected_idx = find_changed_point_index(current_points)
		if self.alt_mode_selected_idx == -1:
			return
		self.alt_mode_affected_points = find_points_to_alt_mode_snap(self.alt_mode_selected_idx)
		alt_mode_saved_configurations.clear()
		self.is_target_of_alt_mode_snapping = true
		alt_mode_saved_configurations[self] = last_points
		for p in self.alt_mode_affected_points:
			p._sector.is_target_of_alt_mode_snapping = true
			alt_mode_saved_configurations[p._sector] = p._sector.polygon
		alt_mode_state = AltModeState.ACTIVE
	
	if alt_mode_state == AltModeState.ACTIVE:
		#print("Active: {0}".format([get_full_name()]))
		if ! is_just_being_edited():
			do_alt_mode_retire(current_points)
			alt_mode_state = AltModeState.NONE
			return
		else:
			for p in alt_mode_affected_points:
				p.global_position = self.get_point_position(alt_mode_selected_idx)
			for s in alt_mode_saved_configurations.keys():
				if s != self:
					s._alt_mode_drag_update()
	
	



func _handle_alt_mode_snapping(points: PackedVector2Array)->bool:
	_alt_mode_snap(points)
	return alt_mode_state == AltModeState.ACTIVE




func is_just_being_edited()->bool:
	return	( (alt_mode_state_is_none(alt_mode_state) && is_target_of_alt_mode_snapping) 
				or (Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT) && NodeUtils.is_the_only_selected_node(self))
			)


static func find_nearest_point(v: Vector2, others: Array[SectorPoint], tolerance: float)-> SectorPoint:
	var toleranceSqr : float = tolerance *tolerance
	var ret : SectorPoint = null
	var ret_distance : float = INF
	for p in others:
		var distance : float = p.global_position.distance_squared_to(v)
		if distance >= toleranceSqr: continue
		if distance < ret_distance:
			#print("({4}) better: {0} < {1} ({2} < {3})".format([distance, ret_distance, p, ret, v]))
			ret = p
			ret_distance = distance
	return ret	



func get_full_name()->String:
	var ret : Array[String] = []
	var node : Node= self
	while (node != null) and (node is not Level):
		ret.append(node.name)
		node = node.get_parent()
	ret.reverse()
	return DatastructUtils.string_concat(ret, "::")
