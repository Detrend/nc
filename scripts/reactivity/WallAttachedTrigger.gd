@tool
class_name WallAttachedTrigger
extends WallTextureOverride


@export_tool_button("Add Trigger") var _add_trigger_tool_button = func()->void: NodeUtils.instantiate_child_by_type_and_select(self, Trigger, "Trigger")


func get_triggers(out: Array[Trigger] = [])->Array[Trigger]:
	return NodeUtils.get_children_of_type(self, Trigger, out)