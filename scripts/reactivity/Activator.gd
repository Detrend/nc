## Entity carrying a counter.
## 
## Its value can be changed during game's runtime by [Trigger]s.
## [IActivatorHook]s can listen for its state and perform some actions based on it.
@tool
class_name Activator
extends Node

## Value that must be reached by the counter for the [Activator] to be considered active.
@export var threshold : int = 1

## Gets name under which this [Activator] shall get exported. Corresponds to the node's name in the scene tree.
func get_activator_name()->StringName: return self.name

## Export all data of this [Activator] in a JSON-serializable object.
func do_export(out: Dictionary = {})->Dictionary:
	out["name"] = get_activator_name()
	out["threshold"] = threshold
	
	var hooks : Array[Dictionary] = []
	for hook : IActivatorHook in NodeUtils.get_children_of_type(self, IActivatorHook):
		var hook_export : Dictionary = {}
		hook.do_export(hook_export)
		hooks.append(hook_export)
	out["hooks"] = hooks
	
	return out
