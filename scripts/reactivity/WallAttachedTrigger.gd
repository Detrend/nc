@tool
class_name WallAttachedTrigger
extends WallTextureOverride


@export_tool_button("Add Trigger") var _add_trigger_tool_button = func()->void: NodeUtils.instantiate_child_by_type_and_select(self, Trigger, "Trigger")


func get_triggers(ret: Array[Trigger] = [])->Array[Trigger]:
	return Trigger.get_triggers_for_node(self, ret) 
