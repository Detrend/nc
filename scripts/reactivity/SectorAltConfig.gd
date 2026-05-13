## Specifies alternative floor/ceiling config that the parent [Sector] can be switched to through an [Activator].
@tool
class_name SectorAltConfig
extends Node

## Switch to this alt config when this [Activator] is enabled.
@export var source : Activator

enum ComputationMode{
	Ignore, Relative, Absolute
}

## How fast floor and ceiling shall move to the default/alt position, in m/s.
@export var move_speed : float = 1.0
@export var floor_mode : ComputationMode = ComputationMode.Ignore
@export var floor_height : float
@export var ceiling_mode : ComputationMode = ComputationMode.Ignore
@export var ceiling_height : float

var _parent_sector : Sector:
	get: return (get_parent() as Sector)


func get_floor_height()->float:
	return _get_some_height_helper(_parent_sector.floor_height, floor_height, floor_mode)

func get_ceiling_height()->float:
	return _get_some_height_helper(_parent_sector.ceiling_height, ceiling_height, ceiling_mode)
	
	
func _get_some_height_helper(parent_height: float, height : float, mode : ComputationMode)->float:
	if mode == ComputationMode.Ignore: return parent_height
	if mode == ComputationMode.Relative: return parent_height + height
	if mode == ComputationMode.Absolute: return height
	ErrorUtils.report_error("Wrong height mode! ({0})".format([name]))
	return parent_height

func is_active()->bool: return source != null

func do_export(ret : Dictionary = {})->Dictionary:
	ret["activator"] = source.get_activator_name()
	ret["floor"] = get_floor_height() * _parent_sector._level.export_scale.z
	ret["ceiling"] = get_ceiling_height() * _parent_sector._level.export_scale.z
	ret["move_speed"] = move_speed
	if move_speed <= 0.0: ErrorUtils.report_warning("{0} - Invalid move speed {1}".format([NodeUtils.get_full_name(self), move_speed]))
	return ret


static func get_alt_configs_for_node(parent: Node, ret: Array[SectorAltConfig] = []) -> Array[SectorAltConfig]:
	return NodeUtils.get_children_by_predicate(parent, func(n:Node)->bool: return n is SectorAltConfig and (n as SectorAltConfig).is_active(), ret)



@warning_ignore("unused_private_class_variable")
@export_tool_button("Swap Heights") var _swap_alt_configs_btn = func()->void:
	var sector := get_parent() as Sector
	if not sector:
		ErrorUtils.report_error("Cannot swap - AltConfig '{0}' is not child of a Sector".format([NodeUtils.get_full_name(self)]))
		return
	var og_floor := sector.floor_height
	var og_ceiling := sector.ceiling_height
	var new_floor := self.get_floor_height()
	var new_ceiling := self.get_ceiling_height()
	sector.floor_height = new_floor
	sector.ceiling_height = new_ceiling
	self.floor_mode = SectorAltConfig.ComputationMode.Absolute
	self.floor_height = og_floor
	self.ceiling_mode = SectorAltConfig.ComputationMode.Absolute
	self.ceiling_height = og_ceiling
			
			
