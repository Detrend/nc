// Project Nucledian Source File
#pragma once

#include <types.h>
#include <engine/entity/entity.h>

#include <math/vector.h>
#include <engine/appearance.h>

#include <string>

namespace nc
{

class Particle : public Entity
{
public:
  static EntityType get_type_static();

  void post_init();

  void update(f32 delta);

  Particle
  (
    vec3   position,
    cstr   sprite,
    u32    num_imgs,
    f32    duration,
    color3 light       = colors::BLACK,
    f32    light_range = 0.0f
  );

  Particle
  (
    vec3   position,
    f32    duration,
    color3 light,
    f32    light_range
  );

  Appearance*       get_appearance();
  const Appearance* get_appearance() const;

private:
  Appearance m_appear;
  EntityID   m_light_opt     = INVALID_ENTITY_ID;
  color3     m_light_color   = colors::BLACK;
  u32        m_img_cnt       = 0;
  f32        m_duration      = 0.0f;
  f32        m_lifetime      = 0.0f;
  f32        m_initial_range = 0.0f;
};

}
