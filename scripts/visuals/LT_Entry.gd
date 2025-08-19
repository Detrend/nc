@tool
class_name LT_Entry
extends ILayeredTextureEntry

@export var texture : ITextureDefinition

@export_range(0.0, 1.0) var anchor_point : float

@export_multiline var height_expression : String = "min(1.0, 0.5 * total_height)":
	get: return height_expression
	set(val):
		height_expression = val
		_height_expression = null

var _height_expression : Expression
func get_height_expression()->Expression:
	if not _height_expression:
		_height_expression = Expression.new()
		_height_expression.parse(self.height_expression, _HEIGHT_EXPRESSION_ARGUMENTS)
	return _height_expression

@export var _arguments_preview : PackedStringArray:
	get: return _HEIGHT_EXPRESSION_ARGUMENTS
	set(val): pass
	
static var _HEIGHT_EXPRESSION_ARGUMENTS : PackedStringArray = [
	'total_height',
	'available_height',
	'wall_height_min',
	'wall_height_max',
	'this_sector_floor',
	'this_sector_ceiling',
	'owner_sector_floor',
	'owner_sector_ceiling',
	'other_sector_floor',
	'other_sector_ceiling',
]

func get_height(total_height: float, available_height: float, texturing_interval : Vector2, ctx : ITextureDefinition.TexturingContext)->float:
	
	var other_sector :Sector = ctx.export_data.get_other_sector(ctx.target_sector)
	var owner_sector :Sector = ctx.get_rule_owner_sector()
	var expr := get_height_expression()
	var ret :float = expr.execute([
		total_height,
		available_height,
		texturing_interval.x,
		texturing_interval.y,
		ctx.target_sector.floor_height,
		ctx.target_sector.ceiling_height,
		owner_sector.floor_height,
		owner_sector.ceiling_height,
		other_sector.floor_height   if other_sector else null,
		other_sector.ceiling_height if other_sector else null,
	])
	return ret


func get_texture()->ITextureDefinition:
	return texture
