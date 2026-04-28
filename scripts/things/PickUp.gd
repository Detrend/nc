@tool
extends Thing
class_name PickUp

## Which in-game pickup type is represented by this. DO NOT MODIFY THIS outside the prefabs
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

## DO NOT MODIFY THESE outside the specific [PickUp] prefab
@export_group("Internal")
## Which in-game pickup type is represented by this. DO NOT MODIFY THIS outside the prefabs
@export var type_id : PickUpType = PickUpType.HpSmall

func get_export_category()->String: return "pickups"

func custom_export(_s: Sector, output: Dictionary)->void:
	super.custom_export(_s, output)
	output["type"] = int(self.type_id)
