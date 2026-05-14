## Helper for automatically creating stairs from a sequence of sectors.   
##
## Goes through its children sectors and assigns them gradually increasing floor/ceiling heights.
@tool
class_name StairMaker
extends Node2D

## If true, affect all recursive subchildren, otherwise just direct Sector children
@export var is_recursive : bool = false

@export_tool_button("Reverse child order") var _do_reverse_child_order_btn = func()->void:
	var children := self.get_children()
	for i in children.size():
		self.move_child(children[children.size() - 1 - i], i)

@export_group("Floor")
## If enabled, [member Sector.floor_height] of children [Sector]s will be managed by this [StairMaker]
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var set_floor : bool = false:
	get: return set_floor
	set(val):
		set_floor = val
		_do_apply()

## Floor height assigned to the first sector
@export var floor_begin : float = 0.0:
	get: return floor_begin
	set(val):
		floor_begin = val
		_do_apply()

## Floor height assigned to the last sector.  
## Computed property (based on [member floor_increment] and number of child sectors) 
@export var floor_end : float:
	get: 
		var stairs := _stair_segments
		return floor_begin + (floor_increment * (stairs.size() - 1) if stairs.size() > 1 else 0)
	set(val):
		var stairs := _stair_segments
		if stairs.size() > 1:
			floor_increment = (val - floor_begin) / (stairs.size() - 1)
			
## Floor height delta between two sectors
@export var floor_increment : float = 0.2:
	get: return floor_increment
	set(val):
		floor_increment = val
		_do_apply()
		
@export_subgroup("Use Curve")
## Instead of using fixed [member floor_increment], interpolate floor heights according to a custom curve.
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var floor_use_curve : bool = false
## Curve to assign floor height from first to last sector.
@export var floor_curve : Curve


@export_group("Ceiling")
## If enabled, [member Sector.ceiling_height] of children [Sector]s will be managed by this [StairMaker]
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var set_ceiling : bool = false:
	get: return set_ceiling
	set(val):
		set_ceiling = val
		_do_apply()
## Ceiling height assigned to the first sector
@export var ceiling_begin : float = 1.5:
	get: return ceiling_begin
	set(val):
		ceiling_begin = val
		_do_apply()
## Ceiling height assigned to the last sector.  
## Computed property (based on `ceiling_increment` and number of child sectors) 
@export var ceiling_end : float:
	get: return ceiling_begin + ceiling_increment * _stair_segments.size()
	set(val):
		ceiling_increment = (val - ceiling_begin) / _stair_segments.size()
## Ceiling height delta between two sectors
@export var ceiling_increment : float = 0.1:
	get: return ceiling_increment
	set(val):
		ceiling_increment = val
		_do_apply()

@export_subgroup("Use Curve")
## Instead of using fixed `ceiling_increment`, interpolate floor heights according to a custom curve.
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var ceiling_use_curve : bool = false
## Curve to assign floor ceiling from first to last sector.
@export var ceiling_curve : Curve


@export_group("Floor Alt")
## If enabled, [member SectorAltConfig.floor_height] of children [Sector]s' [SectorAltConfig] will be managed by this [StairMaker]
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var set_floor_alt : bool = false:
	get: return set_floor_alt
	set(val):
		set_floor_alt = val
		_do_apply()

## Floor height assigned to the first sector alt config
@export var floor_begin_alt : float = 0.0:
	get: return floor_begin_alt
	set(val):
		floor_begin_alt = val
		_do_apply()

## Floor height assigned to the last sector.  
## Computed property (based on [member floor_increment_alt] and number of child sectors) 
@export var floor_end_alt : float:
	get: 
		var stairs := _stair_segments
		return floor_begin_alt + (floor_increment_alt * (stairs.size() - 1) if stairs.size() > 1 else 0)
	set(val):
		var stairs := _stair_segments
		if stairs.size() > 1:
			floor_increment_alt = (val - floor_begin_alt) / (stairs.size() - 1)
			
## Floor height delta between two sectors
@export var floor_increment_alt : float = 0.2:
	get: return floor_increment_alt
	set(val):
		floor_increment_alt = val
		_do_apply()
		
@export_subgroup("Use Curve")
## Instead of using fixed [member floor_increment], interpolate floor heights according to a custom curve.
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var floor_use_curve_alt : bool = false
## Curve to assign floor height from first to last sector.
@export var floor_curve_alt : Curve


@export_group("Ceiling Alt")
## If enabled, [member SectorAltConfig.ceiling_height] of children [Sector]s' [SectorAltConfig] will be managed by this [StairMaker]
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var set_ceiling_alt : bool = false:
	get: return set_ceiling_alt
	set(val):
		set_ceiling_alt = val
		_do_apply()
## Ceiling height assigned to the first sector
@export var ceiling_begin_alt : float = 1.5:
	get: return ceiling_begin_alt
	set(val):
		ceiling_begin_alt = val
		_do_apply()
## Ceiling height assigned to the last sector.  
## Computed property (based on `ceiling_increment` and number of child sectors) 
@export var ceiling_end_alt : float:
	get: return ceiling_begin_alt + ceiling_increment_alt * _stair_segments.size()
	set(val):
		ceiling_increment_alt = (val - ceiling_begin_alt) / _stair_segments.size()
## Ceiling height delta between two sectors
@export var ceiling_increment_alt : float = 0.1:
	get: return ceiling_increment_alt
	set(val):
		ceiling_increment_alt = val
		_do_apply()

@export_subgroup("Use Curve")
## Instead of using fixed `ceiling_increment`, interpolate floor heights according to a custom curve.
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var ceiling_use_curve_alt : bool = false
## Curve to assign floor ceiling from first to last sector.
@export var ceiling_curve_alt : Curve



var _stair_segments : Array[EditablePolygon]:
	get:
		var flags := NodeUtils.LOOKUP_FLAGS.NONE
		if is_recursive: flags |= NodeUtils.LOOKUP_FLAGS.RECURSIVE
		return EditablePolygon.get_all_editable(self, [], flags)


func _ready() -> void: 
	_do_apply()


func on_descendant_editing_start(_ancestor: Node)->void:
	EditablePolygon.on_descendant_editing_start_base(self, _ancestor)
	_do_apply()

func on_descendant_editing_finish(_ancestor: Node, _start_was_called_first: bool):
	EditablePolygon.on_descendant_editing_finish_base(self, _ancestor, _start_was_called_first)
	_do_apply()

func _do_apply_thing(begin: float, increment: float, end: float, to_set : StringName, should_use_curve : bool, curve: Curve, is_alt : bool)->void:
	if (! should_use_curve) || (curve == null):
		var current := begin
		for s in _stair_segments:
			_do_set_thing(s, to_set, current, is_alt)
			current += increment
	else:
		var segments := _stair_segments
		var segments_count_inv :float = 1.0/(segments.size() - 1)
		var current := 0
		for s in segments:
			var t :float = curve.sample(current *segments_count_inv)
			var value :float = lerp(begin, end, t)
			_do_set_thing(s, to_set, value, is_alt)
			current += 1

static func _do_set_thing(s: Node, to_set : StringName, value: float, is_alt : bool)->void:
	if is_alt:
		var alt_config := NodeUtils.get_child_of_type(s, SectorAltConfig) as SectorAltConfig
		if alt_config: alt_config.set(to_set, value)	
	elif s is Sector:
		(s as Sector).set(to_set, value)
	else:
		s.data.set(to_set, value)


func _do_apply()->void:
	if set_floor: _do_apply_thing(floor_begin, floor_increment, floor_end, 'floor_height', floor_use_curve, floor_curve, false)
	if set_ceiling: _do_apply_thing(ceiling_begin, ceiling_increment, ceiling_end, 'ceiling_height', ceiling_use_curve, ceiling_curve, false)
	if set_floor_alt: _do_apply_thing(floor_begin_alt, floor_increment_alt, floor_end_alt, 'floor_height', floor_use_curve_alt, floor_curve_alt, true)
	if set_ceiling_alt: _do_apply_thing(ceiling_begin_alt, ceiling_increment_alt, ceiling_end_alt, 'ceiling_height', ceiling_use_curve_alt, ceiling_curve_alt, true)
