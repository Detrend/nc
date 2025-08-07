@tool
extends Entity
class_name PlayerPosition


func custom_export(_s: Sector, output: Dictionary)->void:
	output["is_player"] = true
