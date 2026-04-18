## Base class for all activator hooks.
##
## Activator hooks are generic objects that can listen to [Activator] state and perform some game logic based on it.
@tool
class_name IActivatorHook
extends Node

## Return some constant string that uniquely identifies this type of activator hook
func get_type()->String: return InterfaceUtils.report_not_implemented_error(get_type)

## Export all info into a JSON-serializable object
func do_export(out: Dictionary)->void:
	out["type"] = get_type();
