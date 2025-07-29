@tool
extends Polygon2D
class_name EditablePolygon


@export_tool_button("Snap points") var snap_points_tool_button = _snap_points

var is_editable : bool = true

var config : EditorConfig = preload("res://config.tres") as EditorConfig


var _level : Level:
	get: return NodeUtils.get_ancestor_component_of_type(self, Level)


func _selected_update(selected_list: Array[Node])->void:
	if selected_list.size() == 1:
		pass
	pass

func _on_sole_selected()->void:
	if not is_editable:
		print("Selected a non-editable node")
		var editable_ancestor : Node = NodeUtils.get_ancestor_by_predicate(self, func(n:Node): return !(n is EditablePolygon) or n.is_editable)
		if editable_ancestor: 
			var selection := EditorInterface.get_selection()
			selection.clear()
			selection.add_node(editable_ancestor)
	pass

func _on_sole_unselected()->void:
	pass

func _on_selected_input(selected_list: Array[Node])->void:
	if selected_list.size() == 1 and _level.is_key_down(KEY_Q):
		pass
	pass

func _on_config_change()->void:
	pass


var last_points : PackedVector2Array = []

func _process(_delta: float) -> void:
	if ! Engine.is_editor_hint(): return
	if ! self.is_visible_in_tree(): return
	
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



var is_target_of_alt_mode_snapping : bool = false
var affected_points : Array[SectorPoint] = []
var selected_idx : int = -1
var last_sector_pos : Vector2 = Vector2.ZERO

func _alt_mode_snap(current_points: PackedVector2Array)->int:
	var last_pos := self.last_sector_pos
	self.last_sector_pos = self.global_position
	if last_sector_pos != self.global_position: return -1 # reset if the whole sector was moved
	if ! Input.is_physical_key_pressed(KEY_Q): return -1 # reset if Q is not pressed
	if current_points.size() != last_points.size(): return -1 # reset if point count changed
	if ! NodeUtils.is_the_only_selected_node(self): return -1 # reset if this is not the single selected node
	
	var max_distance_sqr = config.shared_continuous_movement_max_step * config.shared_continuous_movement_max_step
	var t: int = 0
	var selected :int = -1
	while t < current_points.size():
		if current_points[t] != last_points[t]:
			if selected != -1: return -1 # reset if it's more than one point that changed
			selected = t
		t += 1
	if selected == -1 and is_just_being_edited():
		selected = self.selected_idx
		
	var current_absolute := point_pos_relative_to_absolute(current_points[selected])
	if selected != self.selected_idx:
		#print("!!!!! {0} > {1}".format([selected, self.selected_idx]))
		self.selected_idx = selected
		for p in self.affected_points: p._sector.is_target_of_alt_mode_snapping = false
		self.affected_points.clear()
		var previous_absolute := point_pos_relative_to_absolute(last_points[selected])
		for s in _level.get_editable_polygons():
			if s == self: continue
			for p in s.get_points():
				if p.global_position == previous_absolute and (p.global_position.distance_squared_to(current_absolute) < max_distance_sqr):
					p._sector.is_target_of_alt_mode_snapping = true
					self.affected_points.append(p)
	for p in self.affected_points: 
		p.global_position = current_absolute

	return selected

func _handle_alt_mode_snapping(points: PackedVector2Array)->bool:
	self.selected_idx = _alt_mode_snap(points)
	if self.selected_idx == -1:
		for p in self.affected_points: p._sector.is_target_of_alt_mode_snapping = false
		affected_points.clear()
	if self.selected_idx != -1: 
		print("{1}: {0}".format([self.selected_idx, name]))
	return self.selected_idx != -1




func is_just_being_edited()->bool:
	return	( is_target_of_alt_mode_snapping 
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
