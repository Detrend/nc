// Project Nucledian Source File
#include <game/item.h>
#include <cvars.h>

#include <engine/player/player.h> // for checking if should be picked up

#include <engine/graphics/resources/texture.h> // graphics
#include <engine/appearance.h>

#include <engine/entity/entity_type_definitions.h> // EntityTypes::pickup
#include <engine/sound/sound_system.h>             // Playing sounds

#include <game/item_resources.h>     // PICKUP_NAMES, PICKUP_SOUNDS
#include <game/weapons.h>            // WeaponTypes::

namespace nc
{

constexpr f32 PICKUP_RADIUS = 0.4f;
constexpr f32 PICKUP_HEIGHT = 0.2f;

constexpr u32 PICKUP_AMMO_CNTS[] = {0, 6, 12, 30};
static_assert(ARRAY_LENGTH(PICKUP_AMMO_CNTS) == WeaponTypes::count);

constexpr u32 PICKUP_WEAPON_AMMO_CNTS[] = {0, 4, 10, 20};
static_assert(ARRAY_LENGTH(PICKUP_WEAPON_AMMO_CNTS) == WeaponTypes::count);

//==============================================================================
static WeaponType weapon_type_from_pickup_type(PickupType type)
{
  // This is stupid, but probably easiest
  switch (type)
  {
    case PickupTypes::shotgun:      [[fallthrough]];
    case PickupTypes::shotgun_ammo:
    {
      return WeaponTypes::shotgun;
    }

    case PickupTypes::plasma_rifle: [[fallthrough]];
    case PickupTypes::plasma_ammo:
    {
      return WeaponTypes::plasma_rifle;
    }

    case PickupTypes::nail_gun:     [[fallthrough]];
    case PickupTypes::nail_ammo:
    {
      return WeaponTypes::nail_gun;
    }

    default:
    {
      nc_assert(false, "Pickup type is not a weapon");
      return PickupTypes::count;
    }
  }
}

//==============================================================================
PickUp::PickUp(vec3 position, PickupType my_type, bool on_floor)
: Entity(position, PICKUP_RADIUS, PICKUP_HEIGHT)
, type(my_type)
, snap_to_floor(on_floor)
{
  cstr texture_name = PICKUP_NAMES[this->type];
  f32  scaling      = this->is_weapon() ? 45.0f : 30.0f;

  appear = Appearance
  {
    .sprite  = texture_name,
    .scale   = scaling,
    .mode    = Appearance::SpriteMode::mono,
    .pivot   = Appearance::PivotMode::bottom,
    .scaling = Appearance::ScalingMode::fixed,
  };
}

//==============================================================================
EntityType PickUp::get_type_static()
{
  return EntityTypes::pickup;
}

//==============================================================================
bool PickUp::pickup(Player& player)
{
  bool picked_up = false;

  if (this->is_heal())
  {
    if (player.get_health() != player.get_max_health())
    {
      bool is_small = this->type == PickupTypes::hp_small;
      player.heal(is_small ? CVars::medkit_small_hp : CVars::medkit_large_hp);
      picked_up = true;
    }
  }
  else if (this->is_ammo() || this->is_weapon())
  {
    WeaponType weapon = weapon_type_from_pickup_type(this->type);

    // Give player the weapon
    if (this->is_weapon() && !player.has_weapon(weapon))
    {
      player.give_weapon(weapon);
      picked_up = true;
    }

    // Give him the ammo if not full
    if (player.get_ammo(weapon) != player.get_max_ammo(weapon))
    {
      u32 ammo_cnt = is_ammo()
        ? PICKUP_AMMO_CNTS[weapon]
        : PICKUP_WEAPON_AMMO_CNTS[weapon];

      player.give_ammo(weapon, ammo_cnt);
      picked_up = true;
    }
  }

  if (picked_up)
  {
    SoundSystem::get().play_oneshot(PICKUP_SOUNDS[this->type]);
  }

  return picked_up;
}

//==============================================================================
bool PickUp::snaps_to_floor() const
{
  return this->snap_to_floor;
}

//==============================================================================
bool PickUp::is_heal() const
{
  return this->type == PickupTypes::hp_small
    || this->type == PickupTypes::hp_big;
}

//==============================================================================
bool PickUp::is_weapon() const
{
  return this->type >= PickupTypes::first_weapon
    && this->type <= PickupTypes::last_weapon;
}

//==============================================================================
bool PickUp::is_ammo() const
{
  return this->type >= PickupTypes::first_ammo
    && this->type <= PickupTypes::last_ammo;
}

//==============================================================================
const Appearance& PickUp::get_appearance() const
{
  return const_cast<PickUp*>(this)->get_appearance();
}

//==============================================================================
Appearance& PickUp::get_appearance()
{
  return appear;
}

}
