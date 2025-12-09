@tool
class_name Trigger
extends Node

@export var target : Activator

@export var timeout : float = 0.0
@export var can_turn_off : bool = false
@export var player_sensitive : bool = true
@export var enemy_sensitive : bool = false
@export var while_entity_is_alive : bool = false

func do_export(out : Dictionary)->void:
	out["timeout"] = timeout
	out["can_turn_off"] = can_turn_off
	out["player_sensitive"] = player_sensitive
	out["enemy_sensitive"] = enemy_sensitive
	out["while_alive"] = while_entity_is_alive
