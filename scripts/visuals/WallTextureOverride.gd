@tool
class_name WallTextureOverride
extends WallAttachment

@export var texture : ITextureDefinition
@export var stripe_floor_offset : float = 0.0

@export_group("Use Offset from ceiling")

var __use_ceiling_offset_impl_ : bool = true
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var _use_ceiling_offset :bool:
	get: return __use_ceiling_offset_impl_
	set(val): 
		__use_stripe_width_impl_ = ! val
		__use_ceiling_offset_impl_ = val
@export var ceiling_offset : float = 0.0


@export_group("Use Stripe Width")

var __use_stripe_width_impl_ : bool = false
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var _use_stripe_width :bool:
	get: return __use_stripe_width_impl_
	set(val): 
		__use_stripe_width_impl_ = val
		__use_ceiling_offset_impl_ = ! val
@export var stripe_width : float = 0.0


func apply(out: TexturingResult, og_begin_height: float, og_end_height : float, ctx : TexturingContext)->void:
	if not texture:
		ErrorUtils.report_error("Invalid (nil) texture override on {0}".format([NodeUtils.get_full_name(self)]))
		return

	var begin : float = clampf(og_begin_height + stripe_floor_offset, og_begin_height, og_end_height)
	var end : float = og_end_height
	if end <= begin:
		ErrorUtils.report_warning("Wall override {0} has zero width!".format([NodeUtils.get_full_name(self)]))
		return
	if _use_stripe_width: end = clampf(begin + stripe_width, og_begin_height, og_end_height)
	elif _use_ceiling_offset: end = clampf(og_end_height - ceiling_offset, og_begin_height, og_end_height)

	#print("applying wall override {2}: interval <{0}, {1}> (og: <{3}, {4}>)".format([begin, end, self.name, og_begin_height, og_end_height]))

	var original_entries := out.entries
	out.entries = []
	var override := TexturingResult.new()
	texture.resolve(override, begin, end, ctx)

	for ov in override.entries:
		print("ov-{0}... <{1}, {2}>".format([ov.id, ov.begin_height, ov.end_height]))

	for e in original_entries:
		if e.end_height <= begin or end <= e.begin_height:
			out.entries.append(e)
			continue


		if e.begin_height <= begin and begin <= e.end_height:
			if e.begin_height < begin:
				var lower_half := e.clone()
				lower_half.end_height = begin
				out.entries.append(lower_half)
			if override == null:
				ErrorUtils.report_error("Single texture override used more than once! on {0}".format([NodeUtils.get_full_name(self)]))
			out.entries.append_array(override.entries)
			if end < e.end_height:
				var upper_half := e.clone()
				upper_half.begin_height = end
				out.entries.append(upper_half)
				
			override = null
			continue
		if begin <= e.begin_height and e.end_height <= end:
			continue
		if begin <= e.begin_height and e.begin_height < end and end < e.end_height:
			e.begin_height = end
			out.entries.append(e)
			continue
		ErrorUtils.report_error("This shouldn't happen! - wall override on {0}".format([NodeUtils.get_full_name(self)]))
		
	if override != null:
		ErrorUtils.report_error("This shouldn't happen! - wall override WASN'T APPLIED on {0}".format([NodeUtils.get_full_name(self)]))
		
		
