#pragma once

#include <types.h>
#include <math/vector.h>

#include <engine/entity/entity.h>
#include <engine/entity/entity_types.h>

namespace nc
{

class AmbientLight : public Entity
{
public:
  using Base = Entity;
  static EntityType get_type_static();

  f32 strength;

  AmbientLight(f32 strength);
};

// GPU data friendly with std430 layout
struct DirLightGPU
{
  color3 color;
  f32    intensity;
  vec3   direction;
  f32    _padding;
};

class DirectionalLight : public Entity
{
public:
  using Base = Entity;
  static EntityType get_type_static();

  color3 color     = colors::WHITE;
  vec3   direction = vec3(0.218f, 0.872f, 0.436f);
  f32    intensity = 1.0f;

  DirectionalLight
  (
    const vec3& direction = vec3(0.218f, 0.872f, 0.436f),
    f32 intensity = 1.0f,
    const color3& color = colors::WHITE
  );

  DirLightGPU get_gpu_data() const;
};

// GPU data friendly with std430 layout
struct PointLightGPU
{
  vec3   position;
  f32    intensity;
  color3 color;
  f32    radius;
  f32    falloff;
  u32    sector_id;
  f32    _padding[2];
};

class PointLight : public Entity
{
public:
  using Base = Entity;
  static EntityType get_type_static();

  color3 color     = colors::WHITE;
  f32    intensity = 1.0f;
  f32    radius    = 1.0f;
  f32    falloff   = 1.0f;

  // This is a temporary workaround, because I hated all other alternatives.
  // If the light has a certain snap mode active then this is the offset it is
  // gonna have from floor/ceiling. This is the only entity type for which we
  // want this at the moment and will replace this later by a hopefully better
  // solution.
  f32            snap_offset = 0.0f;
  SectorSnapType snap        = 0;

  // Recomputes the radius of the light from the parameters above and sets it
  // as the entity radius.
  // Do this after you change some of the light parameters or else it will not
  // be properly tracked in the sector to entity mapping.
  void refresh_entity_radius();

  PointLight
  (
    const vec3&   position,   // Position of the light in the world
    f32           radius,     // Radius of the light in meters
    f32           intentsity, // Intensity of the light [0, inf)
    f32           falloff,    // How does the light intensity change with distance
    const color3& color = colors::WHITE
  );

  PointLightGPU get_gpu_data(const vec3& position, u32 stitched_sector_id) const;
};

}
