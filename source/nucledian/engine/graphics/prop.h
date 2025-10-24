// Project Nucledian Source File
#pragma once

#include <types.h>
#include <engine/entity/entity.h>
#include <engine/appearance.h>

#include <string_view>

namespace nc
{

class Prop : public Entity
{
public:
  static EntityType get_type_static();

  Prop(vec3 pos, f32 rad, f32 height, const Appearance& appear);

  Appearance&       get_appearance();
  const Appearance& get_appearance() const;

private:
  Appearance m_appear;
};

}
