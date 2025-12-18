@tool
class_name LevelExporter
extends Object


var _level : Level = null


var level_export : Dictionary = {}
var points_export : Array[PackedFloat32Array] = []
var pickups_export : Array[Dictionary] = []
var entities_export : Array[Dictionary] = []
var points_map : Dictionary[Vector2, int] = {}
var sectors_map : Dictionary[Sector, int] = {}
var walls_map : Dictionary[Vector4, WallExportData] = {}

var all_sectors : Array[Sector] 
var all_things : Array[Thing]


func do_export(level : Level)->void:
	self._level = level
	print("exporting level to: '{0}'".format([_level.export_path]))
	var data := create_level_export_data()
	
	var json := JSON.stringify(data, "\t")
	var file := FileAccess.open(_level.export_path, FileAccess.WRITE)
	if ! file:
		ErrorUtils.report_error("Export failed - couldn't open file {0}".format([_level.export_path]))
		return
	file.store_string(json)
	file.close()
	print("export completed")












func init_datastructs()->void:
	level_export  = {}
	points_export = []
	points_map = {}
	sectors_map = {}
	walls_map = {}
	all_sectors = _level.get_sectors(true);
	
	all_things = _level.get_standalone_things()




func get_point_idx(vec: Vector2)->int:
	var ret = points_map.get(vec)
	if ret == null:
		ret = points_export.size()
		var v := _get_export_coords(vec)
		var point_export : PackedFloat32Array = [v.x, v.y]
		points_export.append(point_export)
		points_map[vec] = ret
	return ret as int

func get_wall_data(a: Vector2, b: Vector2)->WallExportData:
	var idx:= _get_wall_key(a, b)
	var ret = walls_map.get(idx)
	if ret == null:
		ret = WallExportData.new()
		walls_map[idx] = ret
	return ret as WallExportData



class WallExportData:
	# Number of holes participating in the wall, can be arbitrary number
	var holes_count : int = 0

	var sector_a : Sector = null
	var sector_b : Sector = null
	var wall_a : int
	var wall_b : int


	func sectors_count()->int:
		return (1 if sector_a else 0) + (1 if sector_b else 0) 

	func get_other_sector(s: Sector)->Sector:
		if s == sector_a: return sector_b
		return sector_a
	func get_only_sector()->Sector:
		return sector_a

	func get_wall_idx(s: Sector)->int:
		if s == sector_b: return wall_b
		return wall_a

	func register_sector(s: Sector, wall_idx : int)->void:
		if s.exclude_from_export:
			holes_count += 1
		else:
			if sectors_count() == 0: 
				sector_a = s
				wall_a = wall_idx
			elif sectors_count() == 2:
				ErrorUtils.report_error("Appending 3rd sector ({0}) to wall: [{1}, {2}]".format([s.get_full_name(), sector_a.get_full_name(), sector_b.get_full_name()]))
			else: 
				sector_b = s
				wall_b = wall_idx

	func compute_wall_height(this_sector : Sector)->float:
		if sectors_count() == 1:
			return sector_a.ceiling_height - sector_a.floor_height
		if sectors_count() == 0: return 0
		var other_sector = get_other_sector(this_sector)
		var floor_delta :float = other_sector.floor_height - this_sector.floor_height
		var ceiling_delta :float = this_sector.ceiling_height - other_sector.ceiling_height
		return max(floor_delta, ceiling_delta)


	func choose_wall_rule(this_sector: Sector, wall_height : float, wall_type : WallRule.PlacementType)->IWallRule:
		for rule in this_sector.data.material.wall_rules:
			var height_check := (rule.wall_length_range.x < 0) or (rule.wall_length_range[0] <= wall_height and wall_height <= rule.wall_length_range[1])
			var surface_type_check := ((rule._placement_type & WallRule.PlacementType.Any == WallRule.PlacementType.Any)
									   or ((rule._placement_type & wall_type) and sectors_count() == 2)
									   or ((rule._placement_type & WallRule.PlacementType.HoleBorder) and sectors_count() >= 1 and holes_count >= 1)
									   or ((rule._placement_type & WallRule.PlacementType.Wall) and sectors_count() == 1)
			)
			if height_check and surface_type_check:
				return rule

		return this_sector.data.material.wall_default

	func export_wall_data(this_sector : Sector)->Array[Dictionary]:
		if not LevelExporter._temp_ctx: LevelExporter._temp_ctx = TexturingContext.new()
		var ctx := LevelExporter._temp_ctx
		ctx.export_data = self
		ctx.target_sector = this_sector
		ctx.subject_type = TexturingContext.TexturingSubjectType.Wall

		var res := TexturingResult.new()

		if sectors_count() <= 1:
			var wall_height :float = this_sector.ceiling_height - this_sector.floor_height
			var chosen_rule := choose_wall_rule(this_sector, wall_height, WallRule.PlacementType.Wall)
			var chosen_texture := chosen_rule.get_texture()
			ctx.this_wall_rule = chosen_rule
			ctx.other_wall_rule = null
			if not chosen_texture:
				ErrorUtils.report_error("Chosen rule with no texture: '{0}'".format([this_sector.get_full_name()]))
			else:
				chosen_texture.resolve(res, this_sector.floor_height, this_sector.ceiling_height, ctx)
		else:
			# sectors_count() == 2
			var other_sector :Sector= get_other_sector(this_sector)

			var floor_delta :float = other_sector.floor_height - this_sector.floor_height
			if floor_delta > 0:
				var this_floor_rule := choose_wall_rule(this_sector, floor_delta, WallRule.PlacementType.Floor)
				var other_floor_rule := choose_wall_rule(other_sector, floor_delta, WallRule.PlacementType.Floor)
				var chosen_floor_rule := this_floor_rule if (this_floor_rule.get_priority() >= other_floor_rule.get_priority()) else other_floor_rule
				
				ctx.this_wall_rule = this_floor_rule
				ctx.other_wall_rule = other_floor_rule
				var chosen_texture := chosen_floor_rule.get_texture()
				if not chosen_texture:
					ErrorUtils.report_error("Chosen rule with no texture: '{0}'".format([this_sector.get_full_name()]))
				else: chosen_texture.resolve(res, this_sector.floor_height, other_sector.floor_height, ctx)

			var ceiling_delta :float = this_sector.ceiling_height - other_sector.ceiling_height
			if ceiling_delta > 0:
				var   this_ceiling_rule := choose_wall_rule(this_sector, ceiling_delta, WallRule.PlacementType.Ceiling)
				var  other_ceiling_rule := choose_wall_rule(other_sector, ceiling_delta, WallRule.PlacementType.Ceiling)
				var chosen_ceiling_rule := this_ceiling_rule if (this_ceiling_rule.get_priority() >= other_ceiling_rule.get_priority()) else other_ceiling_rule
				
				ctx.this_wall_rule = this_ceiling_rule
				ctx.other_wall_rule = other_ceiling_rule
				var chosen_texture := chosen_ceiling_rule.get_texture()
				if not chosen_texture:
					ErrorUtils.report_error("Chosen rule with no texture: '{0}'".format([this_sector.get_full_name()]))
				else: chosen_texture.resolve(res, other_sector.ceiling_height, this_sector.ceiling_height, ctx)
		
		# apply overrides
		var this_wall_idx := get_wall_idx(this_sector)
		for override in this_sector.get_wall_attachments_of_type(this_wall_idx, WallTextureOverride):
			var additional_processing : Callable = Callable()
			var trigger_parent := override as WallAttachedTrigger
			if trigger_parent:
				var triggers := trigger_parent.get_triggers()
				additional_processing = func(segment: TexturingResult.Entry)->void: segment.triggers.append_array(triggers)
			(override as WallTextureOverride).apply(res, this_sector.floor_height, this_sector.ceiling_height, ctx, additional_processing)
		
		var segments_export := res.export([], ctx)
		
		return segments_export





static func _get_wall_key(wall_begin: Vector2, wall_end: Vector2)->Vector4:
	if wall_begin < wall_end: return Vector4(wall_begin.x, wall_begin.y, wall_end.x, wall_end.y)
	else: return Vector4(wall_end.x, wall_end.y, wall_begin.x, wall_begin.y)



func create_level_export_data() -> Dictionary:	

	init_datastructs()

	Sector.sanity_check_all(_level, all_sectors)
	level_sanity_checks()
	
	var activators_export : Array[Dictionary] = []
	for activator in NodeUtils.get_descendants_of_type(_level, Activator):
		activators_export.append((activator as Activator).do_export({}))
	level_export["activators"] = activators_export
	
	for t in range(all_sectors.size()):
		sectors_map[all_sectors[t]] = t
	
	var host_portals : Dictionary[Sector, Array] = {}
	for s in all_sectors:
		if s.has_portal() and s.is_portal_bidirectional:
			host_portals.get_or_add(s.portal_destination, []).append(s)
	
	for s in _level.get_sectors(false):
		var i:= 0
		var walls_count := s.get_walls_count()
		while i < walls_count:
			var wall_data :WallExportData = get_wall_data(s.get_wall_begin(i), s.get_wall_end(i))
			wall_data.register_sector(s, i)
			i += 1
	
	var sectors_export : Array[Dictionary]
	for sector in all_sectors:
		var sector_export := Dictionary()
		var sector_points : PackedInt32Array = []
		for coords in sector.get_point_positions():
			var coords_idx : int = get_point_idx(coords)
			sector_points.append(coords_idx)

		process_things_in_sector(all_things, sector, level_export)

		sector_export["debug_name"] = sector.name
		sector_export["id"] = sectors_map[sector]
		sector_export["floor"] = sector.data.floor_height * _level.export_scale.z
		sector_export["ceiling"] = sector.data.ceiling_height * _level.export_scale.z
		sector_export["points"] = sector_points

		if not sector.data.material:
			ErrorUtils.report_error("No material for sector '{0}'".format([sector.get_full_name()]))
		sector_export["floor_surface"] = get_texture_config(sector.data.material.floor, TexturingContext.TexturingSubjectType.Floor, sector)
		sector_export["ceiling_surface"] = get_texture_config(sector.data.material.ceiling, TexturingContext.TexturingSubjectType.Ceiling, sector)
		var walls_export : Array[Array] = []
		var wall_idx := 0
		var walls_count = sector.get_walls_count()
		while wall_idx < walls_count:
			var wall_data : WallExportData = get_wall_data(sector.get_wall_begin(wall_idx), sector.get_wall_end(wall_idx))
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
			
		var triggers_export : Array[Dictionary] = []
		for trigger in sector.get_sector_triggers():
			triggers_export.append(trigger.do_export({}))
		if not triggers_export.is_empty(): 
			sector_export["triggers"] = triggers_export
			
		var alt_states_export : Array[Dictionary] = []
		for alt_state in sector.get_sector_alt_configs():
			if not alt_state.is_active(): continue
			alt_states_export.append(alt_state.do_export())
		if not alt_states_export.is_empty():
			print("setting alt states for {0}".format([sector.get_full_name()]))
			sector_export["alt_states"] = alt_states_export

		
		sectors_export.append(sector_export)
	level_export["points"] = points_export
	level_export["sectors"] = sectors_export

	return level_export


static var _temp_array_of_dict : Array[Dictionary] = []
static var _temp_ctx : TexturingContext
static func get_texture_config(texture_def: TextureDefinition, texturing_type: TexturingContext.TexturingSubjectType, sector : Sector)->Dictionary:
	if not texture_def:
		ErrorUtils.report_error("invalid texture def: {0}".format([sector.get_full_name()]))
	if not _temp_array_of_dict: _temp_array_of_dict = []
	if not _temp_ctx: _temp_ctx = TexturingContext.new()
	_temp_ctx.export_data = null
	_temp_ctx.subject_type = texturing_type
	_temp_ctx.target_sector = sector
	_temp_array_of_dict.clear()
	texture_def.append_info(_temp_array_of_dict, NAN, NAN, _temp_ctx)
	var ret :Dictionary = _temp_array_of_dict[0]
	_temp_array_of_dict.clear()
	return ret


func process_things_in_sector(all_things: Array, sector: Sector, export_things: Dictionary)->void:
	for i in Vector3i(all_things.size()-1, -1, -1):
		var current :Thing = all_things[i]		
		var pos := current.global_position
		
		if not current.is_standalone(): continue
		if not sector.contains_point(pos): continue
		
		var height :float
		if current.placement_mode == Things.PlacementMode.Floor: height = sector.floor_height + current.height_offset
		elif current.placement_mode == Things.PlacementMode.Ceiling: height = sector.floor_height - current.height_offset
		elif current.placement_mode == Things.PlacementMode.Absolute: height = current.height_offset
		else: ErrorUtils.report_error("Invalid placement_mode '{0}' for thing {1}".format([current.placement_mode, current]))
		
		process_a_thing(current, sector, pos, height, export_things)
		
		all_things.remove_at(i)


func process_a_thing(current: Thing, sector: Sector, pos: Vector2, height: float, export_things: Dictionary)->void:
		var current_export : Dictionary = {}
		var export_coords_2d = _get_export_coords(pos)
		
		current_export['position'] = [export_coords_2d.x, export_coords_2d.y, height * _level.export_scale.z]
		current.custom_export(sector, current_export)
		var export_destination_raw : Variant = export_things.get(current.get_export_category())
		var export_destination :Array[Dictionary]
		if export_destination_raw == null:
			export_destination = []
		else: export_destination = export_destination_raw
		export_destination.append(current_export)
		export_things[current.get_export_category()] = export_destination
		
		for child_raw in NodeUtils.get_children_by_predicate(current, func(ch)->bool: return (ch is Thing) and (not(ch as Thing).is_standalone())):
			var child : Thing= child_raw as Thing
			var child_pos := child.global_position
			var child_height = height + child.height_offset
			process_a_thing(child, sector, child_pos, child_height, export_things)


func _get_export_coords(p : Vector2)-> Vector2:
	p += _level.export_offset
	return Vector2(-p.x * _level.export_scale.x, -p.y * _level.export_scale.y)


func level_sanity_checks()->void:
	var all_players := _level.get_entities(PlayerPosition)
	if all_players.size() != 1: ErrorUtils.report_error("Invalid number of Player Positions: {0}".format([all_players.size()]))
