@tool
extends Node


@export_tool_button("Add Activator") var _add_activator_tool_button = _add_activator


func _add_activator()->void:
	var child := NodeUtils.instantiate_child_by_type(self, Activator)
	child.name = "Activator"
	NodeUtils.set_selection([child])
