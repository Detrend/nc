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
var all_pickups : Array[PickUp]
var all_entities: Array[Entity]


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
	pickups_export = []
	entities_export = []
	points_map = {}
	sectors_map = {}
	walls_map = {}
	all_sectors = _level.get_sectors(true);
	all_pickups = _level.get_pickups()
	all_entities= _level.get_entities()




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
				return LevelExporter.get_texture_config(rule.texture, true, this_sector)

		return LevelExporter.get_texture_config(chosen_material.wall, true, this_sector)


static func _get_wall_key(wall_begin: Vector2, wall_end: Vector2)->Vector4:
	if wall_begin < wall_end: return Vector4(wall_begin.x, wall_begin.y, wall_end.x, wall_end.y)
	else: return Vector4(wall_end.x, wall_end.y, wall_begin.x, wall_begin.y)



func create_level_export_data() -> Dictionary:	

	init_datastructs()

	Sector.sanity_check_all(_level, all_sectors)
	level_sanity_checks()
	
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
			wall_data.register_sector(s)
			i += 1
	
	var sectors_export : Array[Dictionary]
	for sector in all_sectors:
		var sector_export := Dictionary()
		var sector_points : PackedInt32Array = []
		for coords in sector.get_point_positions():
			var coords_idx : int = get_point_idx(coords)
			sector_points.append(coords_idx)

		process_things_in_sector(all_pickups, sector, pickups_export)
		process_things_in_sector(all_entities, sector, entities_export)

		sector_export["debug_name"] = sector.name
		sector_export["id"] = sectors_map[sector]
		sector_export["floor"] = sector.data.floor_height * _level.export_scale.z
		sector_export["ceiling"] = sector.data.ceiling_height * _level.export_scale.z
		sector_export["points"] = sector_points

		sector_export["floor_surface"] = get_texture_config(sector.data.material.floor, false, sector)
		sector_export["ceiling_surface"] = get_texture_config(sector.data.material.ceiling, false, sector)
		var walls_export : Array[Dictionary] = []
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
		sectors_export.append(sector_export)
	level_export["points"] = points_export
	level_export["sectors"] = sectors_export
	level_export["pickups"] = pickups_export
	level_export["entities"] = entities_export

	return level_export


static func get_texture_config(texture_def: TextureDefinition, is_wall: bool, sector : Sector)->Dictionary:
	if not texture_def:
		ErrorUtils.report_error("invalid texture def: {0}".format([sector.get_full_name()]))
	var ret : Dictionary = {}
	ret["id"] = texture_def.id
	ret["scale"] = texture_def.scale
	ret["show"] = texture_def.should_show
	var base_rotation_deg :float = texture_def.rotation
	var custom_rotation_deg :float = sector.data.wall_texturing_rotation if is_wall else sector.data.texturing_rotation
	ret["rotation"] = deg_to_rad(base_rotation_deg + custom_rotation_deg)
	var offset := sector.data.wall_texturing_offset if is_wall else sector.data.texturing_offset
	ret["offset"] = [offset.x, offset.y]
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
			current_export['position'] = [export_coords_2d.x, export_coords_2d.y, height * _level.export_scale.z]
			current.custom_export(sector, current_export)
			export_things.append(current_export)
			all_things.remove_at(i)
		else:
			i += 1



func _get_export_coords(p : Vector2)-> Vector2:
	p += _level.export_offset
	return Vector2(-p.x * _level.export_scale.x, -p.y * _level.export_scale.y)


func level_sanity_checks()->void:
	var all_players := _level.get_entities(PlayerPosition)
	if all_players.size() != 1: ErrorUtils.report_error("Invalid number of Player Positions: {0}".format([all_players.size()]))
