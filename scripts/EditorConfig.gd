extends Resource
class_name EditorConfig

@export var portal_entry_color : Color = Color.DARK_GREEN
@export var portal_exit_color_single : Color = Color.DARK_RED
@export var portal_exit_color_bidirectional : Color = Color.BLUE

@export var sector_min_height : float = 0.0
@export var sector_min_height_color : Color = Color.WHITE
@export var sector_max_height : float = 5.0
@export var sector_max_height_color : Color = Color.DARK_GRAY

@export var shared_continuous_movement_max_step : float = 10

static func get_floor_color(this: EditorConfig, floor: float)->Color:
	var t = clampf((floor - this.sector_min_height)/(this.sector_max_height - this.sector_min_height), 0.0, 1.0)
	return lerp(this.sector_min_height_color, this.sector_max_height_color, t)
