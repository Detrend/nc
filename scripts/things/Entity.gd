@tool
extends Thing
class_name Entity

@export var entity_type : EntityType = EntityType.Cultist

enum EntityType{
	Cultist=0, Possessed=1, Player=-1
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



@export_tool_button("Add Trigger") var _add_trigger_tool_button = func()->void: NodeUtils.instantiate_child_by_type_and_select(self, Trigger, "Trigger")

func get_triggers(ret : Array[Trigger] = [])->Array[Trigger]:
	return NodeUtils.get_children_of_type(self, Trigger, ret) as Array[Trigger]
