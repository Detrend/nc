## Generic node in the wall texturing process. 
## Can be either a simple [TextureDefinition], or some kind of composite that recursively
##  assembles multiple other [ITextureDefinitions] into a more complex result (e.g. texture stripes), 
##  potentially modifying their results (e.g. adding spatial extrusion or adjusting texture offsets)
@tool
class_name ITextureDefinition
extends Resource

## Append this texture's info into a JSON-serializable object. 
## Mostly for convenience to not break old code.
func append_info(out: Array[Dictionary], begin_height: float, end_height: float, ctx: TexturingContext)->void:
	var result := TexturingResult.new()
	resolve(result, begin_height, end_height, ctx)
	result.export(out, ctx)


## Generate a protocol describing the application of this texture to the given area.
func resolve(out: TexturingResult, begin_height: float, end_height : float, ctx : TexturingContext)->void:
	InterfaceUtils.report_not_implemented_error(resolve)
