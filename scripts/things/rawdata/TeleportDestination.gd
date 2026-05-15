@tool
extends Thing
class_name TeleportDestination

@export var target : Thing

func get_export_category()->String: return Thing.RAW_DATA_EXPORT_CATEGORY

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	if not target:
		ErrorUtils.report_error("No target for TeleportDestination {0}".format([NodeUtils.get_full_name(self)]))
		return
	output["target"] = target._export_tag
