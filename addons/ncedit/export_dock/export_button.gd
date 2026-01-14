@tool
extends Button


func _on_pressed() -> void:
	var edited_root : Node = get_tree().edited_scene_root
	if edited_root is Level:
		edited_root.export_level()
	else:
		print("Currently edited node is not a Level (but '{0}')!".format([edited_root]))
