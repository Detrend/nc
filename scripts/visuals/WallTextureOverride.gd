@tool
class_name WallTextureOverride
extends WallAttachment

@export var texture : ITextureDefinition
@export var is_bidirectional : bool = false
@export var stripe_floor_offset : float = 0.0

@export_group("Use Stripe Width")
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var _use_stripe_width :bool = false:
	get: return _use_stripe_width
	set(val): 
		_use_stripe_width = val
		_use_ceiling_offset = ! val
@export var stripe_width : float = 0.0

@export_group("Use Offset from ceiling")
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var _use_ceiling_offset :bool = false:
	get: return _use_ceiling_offset
	set(val): 
		_use_stripe_width = ! val
		_use_ceiling_offset = val
@export var ceiling_offset : float = 0.0
