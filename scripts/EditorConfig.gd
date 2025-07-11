extends Resource
class_name EditorConfig

enum SectorColoringMode{
	Floor, Ceiling
}

@export var portal_entry_color : Color = Color.DARK_GREEN
@export var portal_exit_color_single : Color = Color.DARK_RED
@export var portal_exit_color_bidirectional : Color = Color.BLUE

@export var floor_min_height : float = 0.0
@export var floor_min_height_color : Color = Color.WHITE
@export var floor_max_height : float = 5.0
@export var floor_max_height_color : Color = Color.DARK_GRAY
@export var floor_color_curve : Curve

@export var ceiling_min_height : float = 1.0
@export var ceiling_min_height_color : Color = Color.WHITE
@export var ceiling_max_height : float = 10.0
@export var ceiling_max_height_color : Color = Color.DARK_GRAY
@export var ceiling_color_curve : Curve

@export var shared_continuous_movement_max_step : float = 10


static func get_sector_color(this: EditorConfig, sector: Sector)->Color:
	var mode:= sector._level.coloring_mode
	if mode == SectorColoringMode.Floor:
		return get_floor_color(this, sector.floor_height)
	if mode == SectorColoringMode.Ceiling:
		return get_ceiling_color(this, sector.ceiling_height)
	ErrorUtils.report_error("this shouldn't happen!")
	return Color.TRANSPARENT

static func get_floor_color(this: EditorConfig, floor: float)->Color:
	var t = clampf((floor - this.floor_min_height)/(this.floor_max_height - this.floor_min_height), 0.0, 1.0)
	if this.floor_color_curve: t = this.floor_color_curve.sample(t)
	return lerp(this.floor_min_height_color, this.floor_max_height_color, t)

static func get_ceiling_color(this: EditorConfig, floor: float)->Color:
	var t = clampf((floor - this.ceiling_min_height)/(this.ceiling_max_height - this.ceiling_min_height), 0.0, 1.0)
	if this.ceiling_color_curve: t = this.ceiling_color_curve.sample(t)
	return lerp(this.ceiling_min_height_color, this.ceiling_max_height_color, t)
