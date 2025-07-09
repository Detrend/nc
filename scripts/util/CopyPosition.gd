extends Node2D

@export var target : Node2D

func _process(delta): _copy_position()
func _physics_process(delta): _copy_position()

func _copy_position():
	if target: self.global_position = target.global_position
