@tool
extends SpinBox

func _ready() -> void:
	var cfg := load("res://config.tres") as EditorConfig
	self.value = cfg.extrude_absolute_override

func _on_value_changed(value: float) -> void:
	var cfg := load("res://config.tres") as EditorConfig
	cfg.extrude_absolute_override = value
