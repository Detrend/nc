@tool
class_name ExtrudedTexture
extends ITextureDefinition

enum ExtrudeMode{
	Plain, AlongSelfEdges, AlongOppositeEdges, AlongOwnerEdges
}

@export var texture : ITextureDefinition

@export var mode : ExtrudeMode = ExtrudeMode.Plain

@export var begin_up_direction : Vector3 = Vector3.ZERO
@export var   end_up_direction : Vector3 = Vector3.ZERO
@export var begin_down_direction : Vector3 = Vector3.ZERO
@export var   end_down_direction : Vector3 = Vector3.ZERO

func append_info(out: Array[Dictionary], begin_height: float, end_height: float, ctx: TexturingContext)->void:
	var idx := out.size()
	texture.append_info(out, begin_height, end_height, ctx)
	
	if mode == ExtrudeMode.Plain:
		_adjust_normals(out, idx, begin_up_direction, end_up_direction, begin_down_direction, end_down_direction)
		return
	
	var owner_sector : Sector
	if mode == ExtrudeMode.AlongSelfEdges:
		owner_sector=  ctx.target_sector
	if mode == ExtrudeMode.AlongOppositeEdges:
		owner_sector = ctx.export_data.get_other_sector(ctx.target_sector)
	if mode == ExtrudeMode.AlongOwnerEdges:
		owner_sector = ctx.get_rule_owner_sector()

	var wall_idx :int = ctx.export_data.get_wall_idx(owner_sector)
	
	var previous_wall_direction := owner_sector.get_wall_direction(wall_idx - 1).normalized()
	var next_wall_direction := -owner_sector.get_wall_direction(wall_idx + 1).normalized()

	var begin_normal :Vector2= previous_wall_direction.normalized()#(GeometryUtils.get_orthogonal(previous_wall_direction) + GeometryUtils.get_orthogonal(wall_direction)).normalized()
	var end_normal :Vector2= next_wall_direction.normalized()#(GeometryUtils.get_orthogonal(wall_direction) + GeometryUtils.get_orthogonal(next_wall_direction)).normalized()
	if owner_sector != ctx.target_sector:
		var temp := begin_normal
		begin_normal = -end_normal
		end_normal = -temp

	var bu :Vector2 = (begin_normal * Vector2(begin_up_direction.x, begin_up_direction.y).length())
	var bd :Vector2 = (begin_normal * Vector2(begin_down_direction.x, begin_down_direction.y).length())
	var eu :Vector2 = (end_normal * Vector2(end_up_direction.x, end_up_direction.y).length())
	var ed :Vector2 = (end_normal * Vector2(end_down_direction.x, end_down_direction.y).length())

	_adjust_normals(out, idx, Vector3(bu.x, bu.y, begin_up_direction.z), Vector3(eu.x, eu.y, end_up_direction.z), Vector3(bd.x, bd.y, begin_down_direction.z), Vector3(ed.x, ed.y, end_down_direction.z),
		func(dict: Dictionary): dict['absolute_directions'] = true
	)

		



func _adjust_normals(container : Array[Dictionary], begin_idx : int, begin_up_direction : Vector3, end_up_direction : Vector3, begin_down_direction : Vector3, end_down_direction : Vector3, additional: Callable = Callable())->void:
	while begin_idx < container.size():
		container[begin_idx].get_or_add('begin_up_direction', TextUtils.vec3_to_array(begin_up_direction))
		container[begin_idx].get_or_add('end_up_direction',  TextUtils.vec3_to_array(end_up_direction))
		container[begin_idx].get_or_add('begin_down_direction',  TextUtils.vec3_to_array(begin_down_direction))
		container[begin_idx].get_or_add('end_down_direction',  TextUtils.vec3_to_array(end_down_direction))
		if additional: additional.call(container[begin_idx])
		begin_idx += 1
