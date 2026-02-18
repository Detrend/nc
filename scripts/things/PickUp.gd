@tool
extends Thing
class_name PickUp

enum PickUpType{
	HpSmall,
	HpBig,
	Shotgun,
	PlasmaRifle,
	NailGun,
	ShotgunAmmo,
	PlasmaAmmo,
	NailAmmo
}

@export_group("Internal")
@export var type_id : PickUpType = PickUpType.HpSmall

func get_export_category()->String: return "pickups"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output["type"] = int(self.type_id)
