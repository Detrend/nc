// Project Nucledian Source File
#pragma once

#include <types.h>       // f32
#include <math/vector.h> // vec3

#include <game/game_types.h> // PickupType

#include <engine/entity/entity.h>
#include <engine/appearance.h>

namespace nc
{

class Player;

class PickUp : public Entity
{
public:
  PickUp(vec3 position, PickupType type, bool snap_to_floor = true);

  static EntityType get_type_static();

  // Will get called if collides with player.
  // Should return true and apply the specific effect or return false and
  // do nothing.
  // Is automatically destroyed after returning true.
  bool pickup(Player& player);

  // Should the pickup lie on the floor, or levitate in the air?
  // If true then the pickup changes height with the floors it lies on.
  bool snaps_to_floor() const;

  bool is_ammo()   const;
  bool is_weapon() const;
  bool is_heal()   const;

  Appearance&       get_appearance();
  const Appearance& get_appearance() const;

private:
  PickupType type;
  Appearance appear;
  bool       snap_to_floor = false;
};

}
