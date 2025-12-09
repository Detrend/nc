@tool
class_name WallAttachedTrigger
extends WallAttachment


@export_tool_button("Add Activator") var _add_trigger_tool_button = _add_trigger


func _add_trigger()->void:
	var child := NodeUtils.instantiate_child_by_type(self, Trigger)
	child.name = "Activator"
	NodeUtils.set_selection([child])
