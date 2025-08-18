@tool
class_name IWallRule
extends Resource

func get_priority()->int:
	return InterfaceUtils.report_not_implemented_error(get_priority)
	
func get_texture()->ITextureDefinition:
	return InterfaceUtils.report_not_implemented_error(get_texture)
