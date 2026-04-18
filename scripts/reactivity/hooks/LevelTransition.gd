## Activator hook that ends the level and transitions to another level
@tool
class_name ActivatorHook_LevelTransition
extends IActivatorHook

## Path identifying the new level
@export var destination : String

func get_type()->String: return "level_transition"

func do_export(out: Dictionary)->void:
	super.do_export(out)
	out["destination"] = destination
