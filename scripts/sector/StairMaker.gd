@tool
extends Node2D


@export var is_recursive : bool = false

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

@export var floor_use_curve : bool = false
@export var floor_curve : Curve


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

@export var ceiling_use_curve : bool = false
@export var ceiling_curve : Curve


var _stair_segments : Array[EditablePolygon]:
	get:
		var flags := NodeUtils.LOOKUP_FLAGS.NONE
		if is_recursive: flags |= NodeUtils.LOOKUP_FLAGS.RECURSIVE
		return EditablePolygon.get_all_editable(self, [], flags)


func _ready() -> void: 
	_do_apply()


func on_descendant_editing_start(_ancestor: EditablePolygon)->void:
	EditablePolygon.on_descendant_editing_start_base(self, _ancestor)
	_do_apply()

func on_descendant_editing_finish(_ancestor: EditablePolygon, _start_was_called_first: bool):
	EditablePolygon.on_descendant_editing_finish_base(self, _ancestor, _start_was_called_first)
	_do_apply()

func _do_apply_thing(should: bool, begin: float, increment: float, end: float, to_set : String, should_use_curve : bool, curve: Curve)->void:
	if should:
		if (! should_use_curve) || (curve == null):
			var current := begin
			for s in _stair_segments:
				if s is Sector:
					s.set(to_set, current)
				else:
					s.data.set(to_set, current)
				current += increment
		else:
			var segments := _stair_segments
			var segments_count_inv :float = 1.0/(segments.size() - 1)
			var current := 0
			for s in segments:
				var t :float = curve.sample(current *segments_count_inv)
				var value :float = lerp(begin, end, t)
				if s is Sector:
					s.set(to_set, value)
				else:
					s.data.set(to_set, value)
				current += 1


func _do_apply()->void:
	_do_apply_thing(set_floor, floor_begin, floor_increment, floor_end, 'floor_height', floor_use_curve, floor_curve)
	_do_apply_thing(set_ceiling, ceiling_begin, ceiling_increment, ceiling_end, 'ceiling_height', ceiling_use_curve, ceiling_curve)
