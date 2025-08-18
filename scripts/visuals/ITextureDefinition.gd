@tool
class_name ITextureDefinition
extends Resource

enum TexturingSubjectType{
	Floor, Ceiling, Wall
}

class TexturingContext:
	var subject_type : TexturingSubjectType
	var export_data : LevelExporter.WallExportData
	var target_sector : Sector
	var this_wall_rule : IWallRule
	var other_wall_rule : IWallRule

func append_info(out: Array[Dictionary], begin_height: float, end_height: float, ctx: TexturingContext)->void:
	InterfaceUtils.report_not_implemented_error(append_info)
