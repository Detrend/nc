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

func resolve(out: TexturingResult, begin_height: float, end_height: float, ctx: TexturingContext)->void:
	var idx := out.entries.size()
	texture.resolve(out, begin_height, end_height, ctx)
	
	if mode == ExtrudeMode.Plain:
		_adjust_normals(out, idx, begin_up_direction, end_up_direction, begin_down_direction, end_down_direction, ctx.level.export_texture_extrude_scale)
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

	_adjust_normals(out, idx, Vector3(bu.x, bu.y, begin_up_direction.z), Vector3(eu.x, eu.y, end_up_direction.z), Vector3(bd.x, bd.y, begin_down_direction.z), Vector3(ed.x, ed.y, end_down_direction.z), ctx.level.export_texture_extrude_scale,
		func(e: TexturingResult.Entry): e.extrude_use_absolute_directions = true
	)

		



static func _adjust_normals(container : TexturingResult, begin_idx : int, begin_up_direction : Vector3, end_up_direction : Vector3, begin_down_direction : Vector3, end_down_direction : Vector3, scale: float, additional: Callable = Callable())->void:
	for i in Vector2i(begin_idx, container.entries.size()):
		var e:= container.entries[i]
		e.is_extruded = true
		e.begin_up_direction = begin_up_direction * scale
		e.end_up_direction = end_up_direction * scale
		e.begin_down_direction = begin_down_direction * scale
		e.end_down_direction = end_down_direction * scale
		if additional: additional.call(e)
