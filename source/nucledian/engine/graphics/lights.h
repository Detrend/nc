#pragma once

#include <types.h>
#include <math/vector.h>

#include <engine/entity/entity.h>
#include <engine/entity/entity_types.h>

namespace nc
{

class DirectionalLight : public Entity
{
public:
  using Base = Entity;
  static EntityType get_type_static();

  static constexpr size_t MAX_DIRECTIONAL_LIGHTS = 8;

  // GPU data friendly with std430 layout
  struct GPUData
  {
    color3 color;
    f32    intensity;
    vec3   direction;
    f32    _padding;
  };

  color3 color     = colors::WHITE;
  vec3   direction = vec3(0.218f, 0.872f, 0.436f);
  f32    intensity = 1.0f;

  DirectionalLight
  (
    const vec3& direction = vec3(0.218f, 0.872f, 0.436f),
    f32 intensity = 1.0f,
    const color3& color = colors::WHITE
  );

  GPUData get_gpu_data() const;
};

//==============================================================================
class PointLight : public Entity
{
public:
  using Base = Entity;
  static EntityType get_type_static();

  static constexpr size_t MAX_VISIBLE_POINT_LIGHTS = 1024;

  // GPU data friendly with std430 layout
  struct GPUData
  {
    vec3   position;
    f32    intensity;
    color3 color;
    f32    constant;
    f32    linear;
    f32    quadratic;
    f32    _padding[2];
  };

  color3 color     = colors::WHITE;
  f32    intensity = 1.0f;
  f32    constant  = 1.0f;
  f32    linear    = 0.09f;
  f32    quadratic = 0.032f;

  PointLight
  (
    const vec3 position,
    f32 intensity = 1.0f,
    f32 constant = 1.0f,
    f32 linear = 0.09f,
    f32 quadratic = 0.032f,
    const color3& color = colors::WHITE
  );

  GPUData get_gpu_data(const vec3& position) const;
  f32 calculate_radius() const;
};

}
