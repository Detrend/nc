## Singleton node that contains all [Activator]s in the [Level] as its children.
@tool
extends Node

## Add a new activator as this node's child
@export_tool_button("Add Activator") var _add_activator_tool_button = func()->void: NodeUtils.instantiate_child_by_type_and_select(self, Activator, "Activator")
