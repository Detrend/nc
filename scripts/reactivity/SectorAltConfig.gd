@tool
class_name SectorAltConfig
extends Node

@export var source : Activator

enum ComputationMode{
	Ignore, Relative, Absolute
}

@export var floor_mode : ComputationMode = ComputationMode.Ignore
@export var floor_height : float
@export var ceiling_mode : ComputationMode = ComputationMode.Ignore
@export var ceiling_height : float

func get_floor_height()->float:
	return _get_some_height((get_parent() as Sector).floor_height, floor_height, floor_mode)

func get_ceiling_height()->float:
	return _get_some_height((get_parent() as Sector).ceiling_height, ceiling_height, ceiling_mode)
	
	
func _get_some_height(parent_height: float, height : float, mode : ComputationMode)->float:
	if mode == ComputationMode.Ignore: return parent_height
	if mode == ComputationMode.Relative: return parent_height + height
	if mode == ComputationMode.Absolute: return height
	ErrorUtils.report_error("Wrong height mode! ({0})".format([name]))
	return parent_height
