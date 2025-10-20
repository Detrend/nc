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
  PickUp(vec3 position, PickupType type);

  static EntityType get_type_static();

  // Will get called if collides with player.
  // Should return true and apply the specific effect or return false and
  // do nothing.
  // Is automatically destroyed after returning true.
  bool pickup(const Player& player);

  Appearance&       get_appearance();
  const Appearance& get_appearance() const;

private:
  PickupType type;
  Appearance appear;
};

}
