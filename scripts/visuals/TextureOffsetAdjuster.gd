@tool
class_name TextureOffsetAdjuster
extends ITextureDefinition

@export var texture : ITextureDefinition

@export var height_expression : String = "0":
	get: return height_expression
	set(val):
		height_expression = val
		_height_expression = null

var _height_expression : Expression = null

func get_height_expression()->Expression:
	if not _height_expression:
		_height_expression = Expression.new()
		_height_expression.parse(self.height_expression, _HEIGHT_EXPRESSION_ARGUMENTS)
	return _height_expression

@export var _arguments_preview: PackedStringArray:
	get: return _HEIGHT_EXPRESSION_ARGUMENTS
	set(val): pass

static var _HEIGHT_EXPRESSION_ARGUMENTS : PackedStringArray = [
	'og_offset_x',
	'og_offset_y',
	'scale',
	'wall_height_min',
	'wall_height_max',
	'stripe_height_min',
	'stripe_height_max',
	'this_sector_floor',
	'this_sector_ceiling',
	'owner_sector_floor',
	'owner_sector_ceiling',
	'other_sector_floor',
	'other_sector_ceiling',
]


func append_info(out: Array[Dictionary], begin_height: float, end_height: float, ctx: TexturingContext)->void:
	var this_sector :Sector = ctx.target_sector
	var other_sector :Sector = ctx.export_data.get_other_sector(this_sector)
	var owner_sector : Sector = ctx.get_rule_owner_sector()
	
	var t :int = out.size()
	texture.append_info(out, begin_height, end_height, ctx)
	var height_expr := get_height_expression()
	while t < out.size():
		var entry : Dictionary = out[t]
		var offset := TextUtils.vec2_from_array(entry['offset'])
		var stripe_height_min :float= entry['begin_height']
		var stripe_height_max :float= entry['end_height']
		var texture_scale :float = entry['scale']
		var height_change :float = height_expr.execute([
			offset.x,
			offset.y,
			texture_scale,
			begin_height,
			end_height,
			stripe_height_min,
			stripe_height_max,
			this_sector.data.floor_height,
			this_sector.data.ceiling_height,
			owner_sector.data.floor_height,
			owner_sector.data.ceiling_height,
			other_sector.data.floor_height if other_sector else null,
			other_sector.data.ceiling_height if other_sector else null,
		])
		var og_offset := offset
		offset.y += height_change
		entry['offset'] = TextUtils.vec2_to_array(offset)
		print("{0}... offset {1} -> {2}".format([this_sector.get_full_name(), og_offset, offset]))
		t += 1
	
