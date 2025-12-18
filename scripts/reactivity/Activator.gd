@tool
class_name Activator
extends Node

#@export var name : StringName
@export var threshold : int = 1

func get_activator_name()->StringName: return self.name
