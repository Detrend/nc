@tool
class_name Activator
extends Node

#@export var name : StringName
@export var threshold : int = 1

func get_activator_name()->StringName: return self.name


func do_export(out: Dictionary = {})->Dictionary:
	out["name"] = get_activator_name()
	out["threshold"] = threshold
	return out
