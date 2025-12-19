@tool
class_name TexturingContext
extends RefCounted

enum TexturingSubjectType{
	Floor, Ceiling, Wall
}

var subject_type : TexturingSubjectType
var export_data : LevelExporter.WallExportData
var target_sector : Sector
var this_wall_rule : IWallRule
var other_wall_rule : IWallRule
var export_scale : Vector3:
	get: return target_sector._level.export_scale
var level : Level:
	get: return target_sector._level

func get_rule_owner_sector()->Sector: 
	if export_data.sectors_count() <= 1:
		return target_sector
	return target_sector if (this_wall_rule.get_priority() >= other_wall_rule.get_priority()) else export_data.get_other_sector(target_sector)
