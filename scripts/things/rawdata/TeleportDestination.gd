@tool
extends Thing
class_name TeleportDestination

@export var target : Thing:
	get: return target
	set(val):
		target = val
		_mimic_a_thing(target)


func _mimic_a_thing(what: Thing)->void:
	var thing_icon := what.get_node_or_null("Icon") as Sprite2D
	var our_icon := self.get_node_or_null("Icon") as Sprite2D
	if our_icon and thing_icon:
		our_icon.texture = thing_icon.texture
		our_icon.scale = thing_icon.scale
	self.name = "D-" + what.name

func get_export_category()->String: return Thing.RAW_DATA_EXPORT_CATEGORY

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	if not target:
		ErrorUtils.report_error("No target for TeleportDestination {0}".format([NodeUtils.get_full_name(self)]))
		return
	output["target"] = target._export_tag
