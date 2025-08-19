@tool
class_name ILayeredTextureEntry
extends Resource

func _init() -> void:
	InterfaceUtils.report_interface_instantiated_error(self, ILayeredTextureEntry)

func get_height(_total_height: float, _available_height: float, _texturing_interval : Vector2, _ctx : ITextureDefinition.TexturingContext)->float:
	return InterfaceUtils.report_not_implemented_error(get_height)

func get_texture()->ITextureDefinition:
	return InterfaceUtils.report_not_implemented_error(get_texture)
