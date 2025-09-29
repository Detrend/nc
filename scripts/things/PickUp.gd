@tool
extends Thing
class_name PickUp

@export_group("Internal")
@export var type_id : int

func get_export_category()->String: return "pickups"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output["type"] = self.type_id
