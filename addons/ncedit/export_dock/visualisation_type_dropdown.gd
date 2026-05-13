@tool
extends OptionButton


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	self.clear()
	for e in EditorConfig.SectorColoringMode.keys():
		self.add_item("%s"%e)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	var edited_root : Node = get_tree().edited_scene_root
	if edited_root is Level:
		self.select(int(edited_root.coloring_mode))
	


func _on_item_selected(index: int) -> void:
	var edited_root : Node = get_tree().edited_scene_root
	if edited_root is Level:
		edited_root.coloring_mode = index as EditorConfig.SectorColoringMode
		#print("Setting coloring mode")
