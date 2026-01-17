@tool
class_name IActivatorHook
extends Node

func get_type()->String: return InterfaceUtils.report_not_implemented_error(get_type)

func do_export(out: Dictionary)->void:
	out["type"] = get_type();
