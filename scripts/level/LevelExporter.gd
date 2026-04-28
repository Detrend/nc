## Encapsulates all logic responsible for exporting a [Level] to a JSON file
@tool
class_name LevelExporter
extends Object

## Level we are exporting
var _level : Level = null

## Root of the JSON document
var level_export : Dictionary = {}

## Array of all [Sector]s that are to be exported
var all_sectors : Array[Sector] 

## Array of all [Thing]s that haven't yet been exported as a member of some [Sector]
var things_awaiting_export : Array[Thing]

## Array of all sector points - each point is stored as an array of size 2. [Sector]s specify their coordinates by indexing into this array.
var points_export : Array[PackedFloat32Array] = []

## Export section containing all [Pickup]s
var pickups_export : Array[Dictionary] = []

## Export section containing all [Entity]s
var entities_export : Array[Dictionary] = []

## Helper structure - mapping: world position => index into the [member points_export] array.
## Do not use this directly, call [method  get_point_idx] instead
var _points_map : Dictionary[Vector2, int] = {}

## Helper structure - mapping: Sector => index into the [member all_sectors] array.
var _sectors_map : Dictionary[Sector, int] = {}

## Helper structure - mapping: WallWorldspacePosition => WallExportData
## Used to recognize when walls are shared between sectors. 
## Wall's begin and end points are lexicographically sorted (so that their order doesn't matter) and then concatenated into a Vector4 - see [method _get_wall_key]
##
## Do not use this directly, call [method get_wall_data] instead
var walls_map : Dictionary[Vector4, WallExportData] = {}


## Perform export of the provided [Level] into a JSON file
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












func _init_datastructs()->void:
	level_export  = {}
	points_export = []
	_points_map   = {}
	_sectors_map  = {}
	walls_map     = {}
	
	all_sectors               = _level.get_sectors(true); # exclude non-export [Sector]s
	things_awaiting_export    = _level.get_standalone_things()



## Get the index of this point in [member points_export]. Create a new entry for it if there isn't any
func get_point_idx(vec: Vector2)->int:
	var ret = _points_map.get(vec)
	if ret == null:
		ret = points_export.size()
		var v := _get_export_coords(vec)
		var point_export : PackedFloat32Array = [v.x, v.y]
		points_export.append(point_export)
		_points_map[vec] = ret
	return ret as int

## Get the [WallExportData] for this sector wall. Create a new entry for it if there isn't any
func get_wall_data(a: Vector2, b: Vector2)->WallExportData:
	var idx:= _get_wall_key(a, b)
	var ret = walls_map.get(idx)
	if ret == null:
		ret = WallExportData.new()
		walls_map[idx] = ret
	return ret as WallExportData


static func _get_wall_key(wall_begin: Vector2, wall_end: Vector2)->Vector4:
	if wall_begin < wall_end: return Vector4(wall_begin.x, wall_begin.y, wall_end.x, wall_end.y)
	else: return Vector4(wall_end.x, wall_end.y, wall_begin.x, wall_begin.y)



## Manages texturing logic for [Sector] walls
class WallExportData:
	# Number of holes participating in the wall, can be arbitrary number
	var holes_count : int = 0

	var sector_a : Sector = null
	var sector_b : Sector = null
	var wall_a : int
	var wall_b : int


	func sectors_count()->int:
		assert(!((sector_a == null) and (sector_b != null)))
		return (1 if sector_a else 0) + (1 if sector_b else 0) 

	func get_other_sector(s: Sector)->Sector:
		assert((s == sector_a) or (s == sector_b))
		return sector_a if (s == sector_b) else sector_b
		
	func get_only_sector()->Sector:
		assert(sector_b == null)
		return sector_a

	func get_wall_idx(s: Sector)->int:
		if s == sector_b: return wall_b
		assert(s == sector_a)
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
				

	func _choose_wall_rule(this_sector: Sector, wall_height : float, wall_type : WallRule.PlacementType)->IWallRule:
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


	## Generate a JSON-exportable object describing the wall textures for this wall in respect to specified [Sector]
	func export_wall_data(this_sector : Sector)->Array[Dictionary]:
		if not LevelExporter._temp_ctx: LevelExporter._temp_ctx = TexturingContext.new()
		var ctx := LevelExporter._temp_ctx
		ctx.clear()
		ctx.export_data = self
		ctx.target_sector = this_sector
		ctx.subject_type = TexturingContext.TexturingSubjectType.Wall

		var res := TexturingResult.new()

		if sectors_count() <= 1:
			# Single sector - wall goes from floor to ceiling
			assert(sectors_count() == 1)
			assert(this_sector == get_only_sector())
			var wall_height    :float = this_sector.ceiling_height - this_sector.floor_height
			var chosen_rule    := _choose_wall_rule(this_sector, wall_height, WallRule.PlacementType.Wall)
			var chosen_texture := chosen_rule.get_texture()
			ctx.this_wall_rule = chosen_rule
			ctx.other_wall_rule = null
			if not chosen_texture:
				ErrorUtils.report_error("Chosen rule with no texture: '{0}'".format([this_sector.get_full_name()]))
			else:
				chosen_texture.resolve(res, this_sector.floor_height, this_sector.ceiling_height, ctx)
		else:
			# Two neighboring sectors - wall goes between floor and ceiling deltas, with empty space in the middle
			assert(sectors_count() == 2)
			var other_sector :Sector= get_other_sector(this_sector)

			var floor_delta :float = other_sector.floor_height - this_sector.floor_height
			if floor_delta > 0:
				var this_floor_rule   := _choose_wall_rule(this_sector, floor_delta, WallRule.PlacementType.Floor)
				var other_floor_rule  := _choose_wall_rule(other_sector, floor_delta, WallRule.PlacementType.Floor)
				var chosen_floor_rule := this_floor_rule if (this_floor_rule.get_priority() >= other_floor_rule.get_priority()) else other_floor_rule
				
				ctx.this_wall_rule = this_floor_rule
				ctx.other_wall_rule = other_floor_rule
				var chosen_texture := chosen_floor_rule.get_texture()
				if not chosen_texture:
					ErrorUtils.report_error("Chosen rule with no texture: '{0}'".format([this_sector.get_full_name()]))
				else: chosen_texture.resolve(res, this_sector.floor_height, other_sector.floor_height, ctx)

			var ceiling_delta :float = this_sector.ceiling_height - other_sector.ceiling_height
			if ceiling_delta > 0:
				var this_ceiling_rule   := _choose_wall_rule(this_sector, ceiling_delta, WallRule.PlacementType.Ceiling)
				var other_ceiling_rule  := _choose_wall_rule(other_sector, ceiling_delta, WallRule.PlacementType.Ceiling)
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
		ctx.clear()
		
		return segments_export






## Generate a JSON-exportable object describing the whole level
func create_level_export_data() -> Dictionary:	

	_init_datastructs() # Fill [member all_sectors] and [member all_things], init everything else to empty

	# run all sanity checks
	Sector.sanity_check_all(_level, all_sectors)
	level_sanity_checks()
	
	
	level_export["music"] = _level.music
	
	# Process all [Activator]s early so that we can safely refer to them later
	var activators_export : Array[Dictionary] = []
	for activator in NodeUtils.get_descendants_of_type(_level, Activator):
		activators_export.append((activator as Activator).do_export({}))
	level_export["activators"] = activators_export
	
	# Fill [member _sectors_map] so that we can safely refer to [Sector]s by their indices throughout rest of the export process
	for t in range(all_sectors.size()):
		_sectors_map[all_sectors[t]] = t
	
	# Take note which [Sector]s are a destination of someone else's portal
	var host_portals : Dictionary[Sector, Array] = {}
	for s in all_sectors:
		if s.has_portal() and s.is_portal_bidirectional:
			(host_portals.get_or_add(s.portal_destination, []) as Array).append(s)
	
	# Create texturing data for all sector walls
	for s in _level.get_sectors(false): # explicitly go through even [Sector]s that are excluded from export - those have special meaning for the [WallExportData.register_sector] function
		var i:= 0
		var walls_count := s.get_walls_count()
		while i < walls_count:
			var wall_data :WallExportData = get_wall_data(s.get_wall_begin(i), s.get_wall_end(i))
			wall_data.register_sector(s, i)
			i += 1
	
	# Now go through all [Sector]s and export them one by one. 
	# Also export all [Thing]s that are contained in some [Sector] into their respective sections. [Thing]s that are outside any [Sector] will get skipped.
	var sectors_export : Array[Dictionary] # Fill this with all sector descriptors
	for sector in all_sectors:
		var sector_export : Dictionary = {}
		
		sector_export["debug_name"] = sector.get_full_name()
		sector_export["id"] = _sectors_map[sector]
		sector_export["floor"] = sector.data.floor_height * _level.export_scale.z
		sector_export["ceiling"] = sector.data.ceiling_height * _level.export_scale.z
		
		var sector_points : PackedInt32Array = []
		for coords in sector.get_point_positions():
			var coords_idx : int = get_point_idx(coords)
			sector_points.append(coords_idx)
		sector_export["points"] = sector_points

		process_things_in_sector(sector, level_export) # Export all [Thing]s that are contained in this [Sector]

		# Export texturing info
		if not sector.data.material:
			ErrorUtils.report_error("No material for sector '{0}'".format([sector.get_full_name()]))
		sector_export["floor_surface"] = export_texture_data(sector.data.material.floor, TexturingContext.TexturingSubjectType.Floor, sector)
		sector_export["ceiling_surface"] = export_texture_data(sector.data.material.ceiling, TexturingContext.TexturingSubjectType.Ceiling, sector)
		var walls_export : Array[Array] = []
		for wall_idx in sector.get_walls_count():
			var wall_data : WallExportData = get_wall_data(sector.get_wall_begin(wall_idx), sector.get_wall_end(wall_idx))
			walls_export.append(wall_data.export_wall_data(sector))
		sector_export["wall_surfaces"] = walls_export

		# Export portal info
		sector_export["portal_target"] = _sectors_map[sector.portal_destination] if sector.has_portal() else 65535
		sector_export["portal_wall"] = sector.portal_wall if sector.has_portal() else -1
		sector_export["portal_destination_wall"] = sector.portal_destination_wall if sector.has_portal() else 0
		var hosts = host_portals.get(sector) # [Array] or [null]
		if hosts != null and !(hosts as Array).is_empty():
			if sector.has_portal(): ErrorUtils.report_error("Sector {0} already has its own portal, but also has host portal from {1}".format([sector, hosts]))
			elif (hosts as Array).size() > 1: ErrorUtils.report_error("Sector {0} has {1} host portals - currently unsupported!".format([sector, (hosts as Array).size()]))
			var host :Sector = hosts[0]
			sector_export["portal_target"] = _sectors_map[host]
			sector_export["portal_wall"] = host.portal_destination_wall
			sector_export["portal_destination_wall"] = host.portal_wall
			
		# Export triggers attached to this sector
		var triggers_export : Array[Dictionary] = []
		for trigger in sector.get_sector_triggers():
			triggers_export.append(trigger.do_export({}))
		if not triggers_export.is_empty(): 
			sector_export["triggers"] = triggers_export
			
		# Export alt states of this sector
		var alt_states_export : Array[Dictionary] = []
		for alt_state in sector.get_sector_alt_configs():
			if not alt_state.is_active(): continue
			alt_states_export.append(alt_state.do_export())
		if not alt_states_export.is_empty():
			if alt_states_export.size() > 1:
				ErrorUtils.report_warning("Currently only 1 alt state is supported by the game, but {0} has {1} of them!".format([sector.get_full_name(), alt_states_export.size()]))
			sector_export["alt_states"] = alt_states_export
		
		# Sector export is complete - append it to the list of all exported sectors 
		sectors_export.append(sector_export)
		
	level_export["points"] = points_export
	level_export["sectors"] = sectors_export

	return level_export


static var _temp_array_of_dict : Array[Dictionary] = [] ## Pooled array of dictionaries to reduce memory waste. Make sure to not use it from two places at once and to cleanup after yourself
static var _temp_ctx : TexturingContext ## Pooled [TexturingContext] to reduce memory waste. Make sure to not use it from two places at once and to cleanup after yourself

## Generate export object describing given texturing surface. 
##
## Used only for simple scenarios like floor and ceiling, not for walls.
static func export_texture_data(texture_def: TextureDefinition, texturing_type: TexturingContext.TexturingSubjectType, sector : Sector)->Dictionary:
	if not texture_def:
		ErrorUtils.report_error("No texture definition for subject {0} at '{1}'".format([texturing_type, sector.get_full_name()]))
	if not _temp_array_of_dict: _temp_array_of_dict = []
	if not _temp_ctx: _temp_ctx = TexturingContext.new()
	_temp_ctx.export_data = null
	_temp_ctx.subject_type = texturing_type
	_temp_ctx.target_sector = sector
	_temp_array_of_dict.clear()
	texture_def.append_info(_temp_array_of_dict, NAN, NAN, _temp_ctx)
	assert(_temp_array_of_dict.size() == 1)
	var ret :Dictionary = _temp_array_of_dict[0]
	_temp_array_of_dict.clear()
	_temp_ctx.clear()
	return ret

## Go through all [Thing]s that are yet to be exported and if they are placed inside this [Sector], export them
func process_things_in_sector(sector: Sector, export_things: Dictionary)->void:
	for i in Vector3i(things_awaiting_export.size()-1, -1, -1):
		var current :Thing = things_awaiting_export[i]		
		var pos := current.global_position
		
		assert(current.is_standalone()) # [member all_things] only contains standalone [Thing]s
		if not sector.contains_point(pos): continue
		
		var height :float
		if current.placement_mode == Things.PlacementMode.Floor: height = sector.floor_height + current.height_offset
		elif current.placement_mode == Things.PlacementMode.Ceiling: height = sector.ceiling_height - current.height_offset
		elif current.placement_mode == Things.PlacementMode.Absolute: height = current.height_offset
		else: ErrorUtils.report_error("Invalid placement_mode '{0}' for a standalone Thing '{1}'".format([current.placement_mode, current.get_full_name()]))
		
		process_a_thing(current, sector, pos, height, export_things)
		
		things_awaiting_export.remove_at(i) # Remove this thing from the list so that we don't have to go through it next time since it's already exported


## Recursively export a specific [Thing] and all its nested subchildren into their designated export sections.
func process_a_thing(current: Thing, sector: Sector, pos: Vector2, height: float, export_things: Dictionary)->void:
		var current_export : Dictionary = {}
		var export_coords_2d = _get_export_coords(pos)
		
		current_export['position'] = [export_coords_2d.x, export_coords_2d.y, height * _level.export_scale.z]
		current.custom_export(sector, current_export)
		var export_destination : Array[Dictionary] = []
		export_destination = export_things.get_or_add(current.get_export_category(), export_destination) as Array[Dictionary]
		export_destination.append(current_export)
		export_things[current.get_export_category()] = export_destination
		
		for child : Thing in NodeUtils.get_children_by_predicate(current, func(ch)->bool: return (ch is Thing) and (not(ch as Thing).is_standalone())):
			var child_pos := child.global_position
			var child_height := height + child.height_offset
			process_a_thing(child, sector, child_pos, child_height, export_things)

## From raw point inside the level (not yet multiplied by [member Level.export_scale] etc.) get a point that can be exported 
func _get_export_coords(p : Vector2)-> Vector2:
	p += _level.export_offset
	return Vector2(-p.x * _level.export_scale.x, -p.y * _level.export_scale.y)


## Perform sanity checks that need to be done globally over the whole level
func level_sanity_checks()->void:
	var all_players := _level.get_entities(PlayerPosition)
	if all_players.size() != 1: ErrorUtils.report_error("Invalid number of Player Positions: {0}".format([all_players.size()]))
