@tool
class_name DefaultWallRule
extends IWallRule

@export var priority : int = 0
@export var texture: ITextureDefinition



func get_priority()->int:
	return priority
	
func get_texture()->ITextureDefinition:
	return texture
