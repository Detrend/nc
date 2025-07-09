extends Node


var seconds_elapsed :float = 0.0

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	seconds_elapsed += delta
