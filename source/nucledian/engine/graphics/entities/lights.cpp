#include <engine/graphics/entities/lights.h>

#include <common.h>

#include <engine/game/game_system.h>
#include <engine/entity/entity_type_definitions.h>

namespace nc
{

constexpr f32 RADIUS_COEFF = 0.23f;

//==============================================================================
EntityType AmbientLight::get_type_static()
{
  return EntityTypes::ambient_light;
}

//==============================================================================
AmbientLight::AmbientLight(f32 strength)
  : Entity(VEC3_ZERO, 0.0f, 0.0f), strength(strength)
{
  static bool alreadyCreated = false;

  if (alreadyCreated)
  {
    nc_warn("More than 1 ambient light entity created.");
  }
  alreadyCreated = true;
}

//==============================================================================
EntityType DirectionalLight::get_type_static()
{
  return EntityTypes::directional_light;
}

//==============================================================================
DirectionalLight::DirectionalLight(const vec3& direction, f32 intensity, const color3& color)
  : Entity(VEC3_ZERO, 0.0f, 0.0f), color(color), direction(direction), intensity(clamp(intensity, 0.0f, 1.0f))
{
  if (intensity < 0.0f || intensity > 1.0f)
  {
    nc_warn("Light intensity is clamped because it is outside [0, 1] range.");
  }
}

//==============================================================================
DirLightGPU DirectionalLight::get_gpu_data() const
{
  return DirLightGPU
  {
    .color = color,
    .intensity = intensity,
    .direction = direction,
  };
}

//==============================================================================
EntityType PointLight::get_type_static()
{
  return EntityTypes::point_light;
}

//==============================================================================
PointLight::PointLight
(
  const vec3&   position,
  f32           radius,
  f32           intensity,
  f32           falloff,
  const color3& color
)
:
  Entity(position, radius),
  color(color),
  radius(radius),
  intensity(intensity),
  falloff(falloff)
{
  if (radius > 16.0f)
  {
    nc_warn("Light radius is too large. It could lead to bad performance. Consider tweaking light parameters.");
  }
}

//==============================================================================
PointLightGPU PointLight::get_gpu_data(const vec3& position, u32 stitched_sector_id) const
{
  return PointLightGPU
  {
    .position  = position,
    .intensity = intensity,
    .color     = color,
    .radius    = radius,
    .falloff   = falloff,
    .sector_id = stitched_sector_id,
    //GameSystem::get().get_map().get_sector_from_point(position.xy),
  };
}

//==============================================================================
void PointLight::refresh_entity_radius()
{
  this->set_radius(radius);
}

}
