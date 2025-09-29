extends Node2D
class_name Thing


@export var placement_mode: Things.PlacementMode = Things.PlacementMode.Floor
@export var height_offset : float = 0.1

func get_export_category()->String: return "misc"

func custom_export(_s: Sector, _output: Dictionary)->void:
	pass
