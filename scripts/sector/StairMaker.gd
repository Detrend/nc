@tool
extends Node2D


@export_group("Floor")
@export var set_floor : bool = false:
	get: return set_floor
	set(val):
		set_floor = val
		_do_apply()

@export var floor_begin : float = 0.0:
	get: return floor_begin
	set(val):
		floor_begin = val
		_do_apply()

@export var floor_end : float:
	get: 
		var stairs := _stair_segments
		return floor_begin + (floor_increment * (stairs.size() - 1) if stairs.size() > 1 else 0)
	set(val):
		var stairs := _stair_segments
		if stairs.size() > 1:
			floor_increment = (val - floor_begin) / (stairs.size() - 1)
@export var floor_increment : float = 0.2:
	get: return floor_increment
	set(val):
		floor_increment = val
		_do_apply()


@export_group("Ceiling")
@export var set_ceiling : bool = false:
	get: return set_ceiling
	set(val):
		set_ceiling = val
		_do_apply()
@export var ceiling_begin : float = 1.5:
	get: return ceiling_begin
	set(val):
		ceiling_begin = val
		_do_apply()
@export var ceiling_end : float:
	get: return ceiling_begin + ceiling_increment * _stair_segments.size()
	set(val):
		ceiling_increment = (val - ceiling_begin) / _stair_segments.size()
@export var ceiling_increment : float = 0.1:
	get: return ceiling_increment
	set(val):
		ceiling_increment = val
		_do_apply()



var _stair_segments : Array[Sector]:
	get: return Sector.get_sectors(self, [], false, NodeUtils.LOOKUP_FLAGS.NONE)


func _ready() -> void: 
	_do_apply()


func on_descendant_editing_start(_ancestor: EditablePolygon)->void:
	EditablePolygon.on_descendant_editing_start_base(self, _ancestor)
	_do_apply()

func on_descendant_editing_finish(_ancestor: EditablePolygon, _start_was_called_first: bool):
	EditablePolygon.on_descendant_editing_finish_base(self, _ancestor, _start_was_called_first)
	_do_apply()


func _do_apply()->void:
	if set_floor:
		var current := floor_begin
		for s in _stair_segments:
			s.floor_height = current
			current += floor_increment
	if set_ceiling:
		var current := floor_begin
		for s in _stair_segments:
			s.floor_height = current
			current += floor_increment
