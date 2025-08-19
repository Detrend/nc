@tool
extends Entity
class_name PlayerPosition


func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output["is_player"] = true
