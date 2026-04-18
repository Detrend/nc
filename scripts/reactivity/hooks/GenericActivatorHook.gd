## Generic hook that carries arbitrary user-provided tree of data.
##
## Use this for rapid prototyping only.
@tool
class_name ActivatorHook_Generic
extends IActivatorHook

## Constant string uniquely identifying this type of activator hook
@export var type : String

## Data to be exported for this activator hook
@export var data : Dictionary[String, Variant]

func get_type()->String: return type

func do_export(out: Dictionary)->void:
	super.do_export(out)
	for k in data:
		if out.has(k): 
			ErrorUtils.report_error("Duplicit key '{0}' in {1}".format([k, NodeUtils.get_full_name(self)]))
			continue
		out[k] = TextUtils.sanitize_json_object(data[k])
