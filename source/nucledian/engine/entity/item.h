// Project Nucledian Source File
#pragma once

#include <types.h>       // f32
#include <math/vector.h> // vec3

#include <engine/entity/entity.h>
#include <engine/appearance.h>

namespace nc
{

class Player;

class PickUp : public Entity
{
public:
  PickUp(vec3 position);

  static EntityType get_type_static();

  virtual void on_pickup(const Player& player);

  Appearance&       get_appearance();
  const Appearance& get_appearance() const;

private:
  Appearance appear;
};

}
