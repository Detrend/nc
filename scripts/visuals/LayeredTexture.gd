class_name LayeredTexture
extends ITextureDefinition


enum CompositionOrder{
	TopToBottom, BottomToTop, SidesToCenter_Topstart, SidesToCenter_Bottomstart, CenterToSides_Topstart, CenterToSides_Bottomstart
}	

@export var composition_order : CompositionOrder

@export var layers : Array[ILayeredTextureEntry]





func append_info(out: Array[Dictionary], begin_height: float, end_height: float, ctx: TexturingContext)->void:
	var out_begin_idx := out.size()
	out.resize(out_begin_idx + layers.size())
	var total_size := end_height - begin_height

	var sizes : PackedVector2Array = DatastructUtils.fill_array_with(PackedVector2Array(), Vector2(NAN, NAN), layers.size()) as PackedVector2Array
	var entries : PackedInt32Array = NodeUtils.get_instance_of_indices(layers, LT_Entry)
	var entry_indices := _generate_indices(composition_order, entries.size())
	for e in entry_indices:
		var entry_idx :int = entries[e]
		var entry : LT_Entry = layers[entry_idx]
		
		var free_size_interval := Vector2(begin_height, end_height)
		var t :int = entry_idx - 1
		while t >= 0:
			if not is_nan(sizes[t].y):
				free_size_interval.x = sizes[t].y
				break
			t -= 1
		t = entry_idx + 1
		while t < sizes.size():
			if not is_nan(sizes[t].y):
				free_size_interval.y = sizes[t].x
				break
			t += 1
		var free_size_interval_size := free_size_interval.y - free_size_interval.x
		var desired_size := entry.get_height(total_size, free_size_interval_size)
		if desired_size >= free_size_interval_size:
			sizes[entry_idx] = free_size_interval
			continue
		var free_size_interval_anchor :float = lerp(free_size_interval.x, free_size_interval.y, entry.anchor_point)
		var half_desired_size := desired_size * 0.5
		var top_space := free_size_interval.y - free_size_interval_anchor
		if top_space <= half_desired_size:
			sizes[entry_idx] = Vector2(free_size_interval.y - desired_size, free_size_interval.y)
			continue
		var bottom_space :=  free_size_interval_anchor - free_size_interval.x
		if bottom_space <= half_desired_size:
			sizes[entry_idx] = Vector2(free_size_interval.x, free_size_interval.x + desired_size)
			continue
		sizes[entry_idx] = Vector2(free_size_interval_anchor - half_desired_size, free_size_interval_anchor + half_desired_size)
		
	if 1: # [just to create a nested scope]
		# now divide the remaining space between all the filler entries
		var t :int = 0
		var first_unpopulated_idx :int = 0
		var last_set_height = begin_height
		while t < sizes.size():
			if not is_nan(sizes[t].x):
				if first_unpopulated_idx < t:
					_divide_interval_to_indices(sizes, first_unpopulated_idx, t, Vector2(last_set_height, sizes[t].x))
				first_unpopulated_idx = t + 1
			t += 1
		_divide_interval_to_indices(sizes, first_unpopulated_idx, sizes.size(), Vector2(last_set_height, end_height))

	if 1:
		var t :int = 0
		while t < layers.size():
			var layer :ILayeredTextureEntry = layers[t]
			var size_interval :Vector2 = sizes[t]
			if size_interval.x < size_interval.y:
				layer.get_texture().append_info(out, size_interval.x, size_interval.y, ctx)
			t += 1



static func _divide_interval_to_indices(sizes: PackedVector2Array, begin_idx : int, end_idx_exclusive: int, sizes_interval: Vector2)->void:
	if begin_idx >= end_idx_exclusive: return

	var height_to_divide :float = sizes_interval.y - sizes_interval.x
	var entries_count := end_idx_exclusive - begin_idx
	var height_per_entry :float= height_to_divide / entries_count
	var to_assign := Vector2(sizes_interval.x, sizes_interval.x + height_per_entry)
	var t := begin_idx
	while t < end_idx_exclusive:
		sizes[t] = to_assign
		to_assign = Vector2(to_assign.y, to_assign.y + height_per_entry)
		t += 1
	sizes[end_idx_exclusive - 1].y = sizes_interval.y # mitigate any rounding errors that might have accumulated previously



static func _generate_indices(order: CompositionOrder, count : int)->PackedInt32Array:
	match order:
		CompositionOrder.TopToBottom:
			return range(count)
		CompositionOrder.BottomToTop:
			var ret := _generate_indices(CompositionOrder.TopToBottom, count)
			ret.reverse()
			return ret
		CompositionOrder.SidesToCenter_Topstart:
			var ret : PackedInt32Array = []
			ret.resize(count)
			var i:int = 0
			while i < count:
				ret[i] = (count - 1 - (i >> 1)) if (i & 1) else (i >> 1)
				i += 1
			return ret
		CompositionOrder.SidesToCenter_Bottomstart:
			var ret : PackedInt32Array = []
			ret.resize(count)
			var i:int = 0
			while i < count:
				ret[i] = (count - 1 - (i >> 1)) if not (i & 1) else (i >> 1)
				i += 1
			return ret
		CompositionOrder.CenterToSides_Topstart:
			var ret := _generate_indices(CompositionOrder.SidesToCenter_Bottomstart, count)
			ret.reverse()
			return ret
		CompositionOrder.CenterToSides_Bottomstart:
			var ret := _generate_indices(CompositionOrder.SidesToCenter_Topstart, count)
			ret.reverse()
			return ret
		_:
			return range(count)