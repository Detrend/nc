## Can be attached to a living [Entity] or to some [Sector]'s floor or wall. When active, affects the counter value of some [Activator]
@tool
class_name Trigger
extends Node

## [Activator] whose value will be modified by this [Trigger]
@export var target : Activator

## In seconds, how long this trigger will remain active after player interacted with it. 0 for infinite.
@export var timeout : float = 0.0

## Whether this trigger can be turned back off after getting turned on once.
@export var can_turn_off : bool = false

## Whether this trigger can be activated by the player
@export var player_sensitive : bool = true

## Whether this trigger can be activated by an enemy
@export var enemy_sensitive : bool = false

## When attached to an [Entity] this flag determined whether the [Trigger] is active while the entity is alive or while it's dead. 
@export var while_entity_is_alive : bool = false

## Export all properties into a JSON-serializable object
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

## Whether this [Trigger] is in a valid state
func is_active()->bool: return target != null

## Cannonical way to obtain all [Trigger]s attached to a specified [Node]
static func get_triggers_for_node(parent: Node, ret: Array[Trigger] = []) -> Array[Trigger]:
	return NodeUtils.get_children_by_predicate(parent, func(n:Node)->bool: return n is Trigger and (n as Trigger).is_active(), ret)
