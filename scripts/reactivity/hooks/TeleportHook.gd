@tool
extends IActivatorHook
class_name TeleportHook

@warning_ignore("unused_private_class_variable")
@export_tool_button("Add Destination") var _add_destination_btn = func()->void:
	NodeUtils.instantiate_child(self, load("res://prefabs/Entities/TeleportDestination.tscn") as PackedScene).name = "Destination"

## Return some constant string that uniquely identifies this type of activator hook
func get_type()->String: return "teleport"

## Export all info into a JSON-serializable object
func do_export(out: Dictionary)->void:
	super.do_export(out)
	var destinations : PackedInt64Array = []
	for destination : TeleportDestination in Thing.get_all_things(self, TeleportDestination):
		destinations.append(destination._export_tag)
	out['destinations'] = destinations
