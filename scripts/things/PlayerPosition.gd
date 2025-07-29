@tool
extends Node2D
class_name PlayerPosition

@export_tool_button("Check") var check_contains_tool_button = check_contains
@export var inclusive: bool = false

func _process(delta: float) -> void:
	return

func check_contains()->void:
	print("----------------------------")
	var _level : Level = get_parent()
	for s in _level.get_sectors():
		if s.contains_point(self.global_position, inclusive):
			print("{0}".format([s.get_full_name()]))
