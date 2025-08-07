@tool
extends Thing
class_name Entity


func custom_export(_s: Sector, output: Dictionary)->void:
	output["is_player"] = false