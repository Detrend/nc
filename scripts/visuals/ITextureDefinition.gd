@tool
class_name ITextureDefinition
extends Resource


func append_info(out: Array[Dictionary], begin_height: float, end_height: float, ctx: TexturingContext)->void:
	var result := TexturingResult.new()
	resolve(result, begin_height, end_height, ctx)
	result.export(out, ctx)

func resolve(out: TexturingResult, begin_height: float, end_height : float, ctx : TexturingContext)->void:
	InterfaceUtils.report_not_implemented_error(resolve)
