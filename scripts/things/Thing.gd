extends Node2D
class_name Thing


@export var placement_mode: Things.PlacementMode = Things.PlacementMode.Floor
@export var height_offset : float = 0.1

@export_group("Nested")
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var is_nested:bool = false

func get_export_category()->String: return "misc"

func custom_export(_s: Sector, _output: Dictionary)->void:
	pass

func is_standalone()->bool:
	return not ((get_parent() is Thing) and is_nested)
