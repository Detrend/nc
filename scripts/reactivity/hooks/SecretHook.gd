## Activator hook that ends the level and transitions to another level
@tool
class_name ActivatorHook_Secret
extends IActivatorHook

## Path identifying the new level

func get_type()->String: return "secret"

func do_export(out: Dictionary)->void:
	super.do_export(out)
