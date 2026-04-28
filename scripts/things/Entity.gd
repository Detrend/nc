## An agent that can move through the game world and interact with it.
##
## It can have [Trigger]s attached to it - those will, depending on their config, 
##  get activated either on the [Entity]'s death or, on the opposite, while it's alive
@tool
extends Thing
class_name Entity

## DO NOT MODIFY THESE outside the specific [Entity] prefab
@export_group("Internal")
## Which in-game entity type is represented by this. DO NOT MODIFY THIS outside the prefabs
@export var entity_type : EntityType = EntityType.Cultist

## Which in-game entity type is represented by a given [Entity]
enum EntityType{
	## Basic ranged enemy
	Cultist=0, 
	## Basic mellee enemy
	Possessed=1, 
	## The player's starting position - there MUST always be exactly one in a level
	Player=-1
}


func get_export_category()->String: return "entities"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output["is_player"] = (entity_type == EntityType.Player)
	output["forward"] = TextUtils.vec3_to_array(_get_forward_direction())
	output["entity_type"] = int(entity_type)
	
	var triggers := get_triggers()
	if not triggers.is_empty():
		var triggers_export : Array[Dictionary] = []
		for trigger in triggers:
			triggers_export.append(trigger.do_export())
		output["triggers"] = triggers_export
	
func _get_forward_direction()->Vector3:
	var rot := Vector2(0.0, -1.0).rotated(self.global_rotation)
	return Vector3(rot.x, rot.y, 0.0)


## Create a new [Trigger] attached to this [Entity]
@export_tool_button("Add Trigger") var _add_trigger_tool_button = func()->void: NodeUtils.instantiate_child_by_type_and_select(self, Trigger, "Trigger")

## Get all [Trigger]s attached to this [Entity] (activated by it being alive/dead)
func get_triggers(ret : Array[Trigger] = [])->Array[Trigger]:
	return Trigger.get_triggers_for_node(self, ret) 
