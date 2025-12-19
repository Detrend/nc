@tool
class_name LayeredTexture
extends ITextureDefinition


@export var layers : Array[ILayeredTextureEntry]


func resolve(out: TexturingResult, total_begin_height: float, total_end_height: float, ctx: TexturingContext)->void:
	var total_size := total_end_height - total_begin_height
	var texturing_interval := Vector2(total_begin_height, total_end_height)
	#print("layers from <{0}, {1}>".format([total_begin_height, total_end_height]))
	
	var sizes : PackedFloat32Array =[]
	sizes.resize(layers.size())

	var available_size := total_size

	# first process normal entries
	var tiers := gather_tiers()
	for tier in tiers:
		#print("found tier: %d (has %d layers)"%[tier, layers.size()])
		var size_used_by_this_tier :float = 0.0
		for i in layers.size():
			var entry := layers[layers.size() - i - 1] as LT_Entry
			if (not entry): continue
			if entry.tier != tier: continue
			var used_size := entry.get_height(total_size, available_size, texturing_interval, ctx)
			sizes[i] = used_size
			size_used_by_this_tier += used_size
			#print("used by tier {0}: {1} (available: {2})".format([tier, used_size, available_size]))
		available_size = max(0, available_size - size_used_by_this_tier)
	
	# last process fillers
	var total_weights : float = 0.0
	var fillers_count : int = 0
	for i in layers.size():
		var filler := layers[layers.size() - i - 1] as LT_Filler
		if not filler: continue
		total_weights += filler.weight
		fillers_count += 1
	if total_weights == 0.0:
		if fillers_count != 0: ErrorUtils.report_warning("{0}... fillers have zero total weight!".format([self.resource_path]))
	else:
		for i in layers.size():
			var filler := layers[layers.size() - i - 1] as LT_Filler
			if not filler: continue
			var weight_normalized := filler.weight / total_weights
			sizes[i] = available_size * weight_normalized


	var current_texturing_begin := total_begin_height
	for i in layers.size():
		var layer := layers[layers.size() - i - 1]
		var size := sizes[i]
		var current_texturing_end := minf(total_end_height, current_texturing_begin + size if i < (layers.size()-1) else total_end_height)
		if current_texturing_end <= current_texturing_begin: 
			#print(" - skip({2}): <{0}, {1}>".format([current_texturing_begin, current_texturing_end, i]))
			continue
		layer.get_texture().resolve(out, current_texturing_begin, current_texturing_end, ctx)
		#print(" - texturing({2}): <{0}, {1}>".format([current_texturing_begin, current_texturing_end, i]))
		current_texturing_begin = current_texturing_end
	#print("END")


func gather_tiers()->PackedInt32Array:
	var ret : PackedInt32Array = []
	for entry in layers:
		var normal_entry := entry as LT_Entry
		if not normal_entry: continue
		if normal_entry.tier not in ret:
			ret.append(normal_entry.tier)
	ret.sort()
	return ret
