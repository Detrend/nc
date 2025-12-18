@tool
class_name Trigger
extends Node

@export var target : Activator

@export var timeout : float = 0.0
@export var can_turn_off : bool = false
@export var player_sensitive : bool = true
@export var enemy_sensitive : bool = false
@export var while_entity_is_alive : bool = false

func do_export(out : Dictionary = {})->Dictionary:
	if not target:
		ErrorUtils.report_error("No target defined for activator {0}".format([NodeUtils.get_full_name(self)]))
		return out
	out["activator"] = target.get_activator_name()
	out["timeout"] = timeout
	out["can_turn_off"] = can_turn_off
	out["player_sensitive"] = player_sensitive
	out["enemy_sensitive"] = enemy_sensitive
	out["while_alive"] = while_entity_is_alive
	return out
