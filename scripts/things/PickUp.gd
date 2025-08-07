@tool
extends Thing
class_name PickUp

@export_group("Internal")
@export var type_id : int


func custom_export(_s: Sector, output: Dictionary)->void:
	output["type"] = self.type_id
