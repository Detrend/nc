@tool
class_name UseTheOtherTextureDefinition
extends ITextureDefinition


func append_info(out: Array[Dictionary], begin_height: float, end_height: float, ctx: TexturingContext)->void:
	if ctx.other_wall_rule.get_texture() == self:
		ErrorUtils.report_error("Cyclic use of OtherTextureDefinitionReference: '{0}'".format([ctx.target_sector.get_full_name()]))
		(load("res://materials/default_texture.tres") as TextureDefinition).append_info(out, begin_height, end_height, ctx)
	ctx.other_wall_rule.get_texture().append_info(out, begin_height, end_height, ctx)
