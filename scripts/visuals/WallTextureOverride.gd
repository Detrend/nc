@tool
class_name WallTextureOverride
extends WallAttachment

@export var texture : ITextureDefinition
@export var stripe_floor_offset : float = 0.0

@export_group("Use Offset from ceiling")

var __use_ceiling_offset_impl_ : bool = true
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var _use_ceiling_offset :bool:
	get: return __use_ceiling_offset_impl_
	set(val): 
		__use_stripe_width_impl_ = ! val
		__use_ceiling_offset_impl_ = val
@export var ceiling_offset : float = 0.0


@export_group("Use Stripe Width")

var __use_stripe_width_impl_ : bool = false
@export_custom(PROPERTY_HINT_GROUP_ENABLE, "") var _use_stripe_width :bool:
	get: return __use_stripe_width_impl_
	set(val): 
		__use_stripe_width_impl_ = val
		__use_ceiling_offset_impl_ = ! val
@export var stripe_width : float = 0.0
