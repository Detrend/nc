@tool
extends Label

func _process(delta: float) -> void:
	if (not get_tree().edited_scene_root) or (not get_tree().edited_scene_root.get_viewport()):
		self.text = ""
		return
	var sectors : Array = NodeUtils.get_selected_nodes_of_type(Sector)
	if sectors.size() != 1:
		self.text = ""
		return
	var sector := sectors[0] as Sector
	var mouse_pos := get_tree().edited_scene_root.get_viewport().get_mouse_position()
	var wall_idx := sector.find_closest_wall_to_point(mouse_pos)
	var wall_length := sector.get_wall_direction(wall_idx).length()
	self.text = "wall {0}: {1}".format([wall_idx, wall_length])
