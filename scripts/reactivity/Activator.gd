@tool
class_name Activator
extends Node

#@export var name : StringName
@export var threshold : int = 1

func get_activator_name()->StringName: return self.name


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
