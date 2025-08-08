@tool
extends Resource
class_name EditorConfig

enum SectorColoringMode{
	Floor, Ceiling, Texturing
}

@export_group("Node editing")
## Max distance at which points can snap into each other
@export var max_snapping_distance : float = 1.0
var _gridsnap_step_impl : float
@export var gridsnap_step : float:
	get:
		if _gridsnap_step_impl <= 0.0:
			_gridsnap_step_impl = 1.0/_gridsnap_step_inv_impl
		return _gridsnap_step_impl
	set(val):
		_gridsnap_step_impl = val
		_gridsnap_step_inv_impl = 1.0/val
var _gridsnap_step_inv_impl : float
@export var gridsnap_step_inv : float = 16:
	get:
		if _gridsnap_step_inv_impl <= 0.0:
			_gridsnap_step_inv_impl = 1.0/_gridsnap_step_impl
		return _gridsnap_step_inv_impl
	set(val):
		_gridsnap_step_inv_impl = val
		_gridsnap_step_impl = 1.0/val

@export var dissolve_removes_self : bool = false
@export var extrude_factor : float = 0.2
		
## How pressing '+' or '-' alters the floor/ceiling height (chosen by current level's coloring mode)
@export var floor_height_increment : float = 0.1
@export var ceiling_height_increment : float = 0.5

@export_group("Level export")
@export var sanity_check_snapping : bool = true
@export var sanity_check_intersecting : bool = true
@export var sanity_check_convex : bool = true

@export_group("Portal visuals")
@export var portal_entry_color : Color = Color.DARK_GREEN
@export var portal_exit_color_single : Color = Color.DARK_RED
@export var portal_exit_color_bidirectional : Color = Color.BLUE

@export_group("Floor visuals")
@export var floor_min_height : float = 0.0:
	get: return floor_min_height
	set(val): floor_min_height = val; on_cosmetics_changed.emit()
@export var floor_min_height_color : Color = Color.WHITE:
	get: return floor_min_height_color
	set(val): floor_min_height_color = val; on_cosmetics_changed.emit()
@export var floor_max_height : float = 5.0:
	get: return floor_max_height
	set(val): floor_max_height = val; on_cosmetics_changed.emit()
@export var floor_max_height_color : Color = Color.DARK_GRAY:
	get: return floor_max_height_color
	set(val): floor_max_height_color = val; on_cosmetics_changed.emit()
@export var floor_color_curve : Curve:
	get: return floor_color_curve
	set(val): floor_color_curve = val; on_cosmetics_changed.emit()

@export_group("Ceiling visuals")
@export var ceiling_min_height : float = 1.0:
	get: return ceiling_min_height
	set(val): ceiling_min_height = val; on_cosmetics_changed.emit()
@export var ceiling_min_height_color : Color = Color.WHITE:
	get: return ceiling_min_height_color
	set(val): ceiling_min_height_color = val; on_cosmetics_changed.emit()
@export var ceiling_max_height : float = 10.0:
	get: return ceiling_max_height
	set(val): ceiling_max_height = val; on_cosmetics_changed.emit()
@export var ceiling_max_height_color : Color = Color.DARK_GRAY:
	get: return ceiling_max_height_color
	set(val): ceiling_max_height_color = val; on_cosmetics_changed.emit()
@export var ceiling_color_curve : Curve:
	get: return ceiling_color_curve
	set(val): ceiling_color_curve = val; on_cosmetics_changed.emit()

@export_group("Misc visuals")
@export var excluded_sector_color : Color = Color(1, 1, 1, 0.3):
	get: return excluded_sector_color
	set(val): excluded_sector_color = val; on_cosmetics_changed.emit()

signal on_cosmetics_changed()

static func get_sector_color(this: EditorConfig, sector: Sector)->Color:
	if sector.exclude_from_export:
		return this.excluded_sector_color
	var mode:= sector._level.coloring_mode
	if mode == SectorColoringMode.Floor:
		return get_floor_color(this, sector.data.floor_height)
	if mode == SectorColoringMode.Ceiling:
		return get_ceiling_color(this, sector.data.ceiling_height)
	ErrorUtils.report_error("this shouldn't happen!")
	return Color.TRANSPARENT

static func get_floor_color(this: EditorConfig, floor: float)->Color:
	var t = clampf((floor - this.floor_min_height)/(this.floor_max_height - this.floor_min_height), 0.0, 1.0)
	if this.floor_color_curve: t = this.floor_color_curve.sample(t)
	return lerp(this.floor_min_height_color, this.floor_max_height_color, t)

static func get_ceiling_color(this: EditorConfig, ceiling: float)->Color:
	var t = clampf((ceiling - this.ceiling_min_height)/(this.ceiling_max_height - this.ceiling_min_height), 0.0, 1.0)
	if this.ceiling_color_curve: t = this.ceiling_color_curve.sample(t)
	return lerp(this.ceiling_min_height_color, this.ceiling_max_height_color, t)
