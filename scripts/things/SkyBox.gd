@tool
class_name SkyBox
extends Thing

@export var texture : String
@export var exposure : float = 1.0
@export var use_gamma_correction : bool = false

func get_export_category()->String: return "skyboxes"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output['texture'] = texture
	output['exposure'] = exposure
	output['use_gamma_correction'] = use_gamma_correction
	
	
