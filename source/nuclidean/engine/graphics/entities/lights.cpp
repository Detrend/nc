// Project Nuclidean Source File
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
void AmbientLight::init(f32 in_strength)
{
  Entity::init(VEC3_ZERO, 0.0f, 0.0f);
  this->strength = in_strength;
}

//==============================================================================
EntityType DirectionalLight::get_type_static()
{
  return EntityTypes::directional_light;
}

//==============================================================================
void DirectionalLight::init(const vec3& in_direction, f32 in_intensity, const color3& in_color)
{
  Entity::init(VEC3_ZERO, 0.0f, 0.0f);
  this->color     = in_color;
  this->direction = in_direction;
  this->intensity = clamp(in_intensity, 0.0f, 1.0f);
  if (in_intensity < 0.0f || in_intensity > 1.0f)
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
/*static*/ EntityStatFlags PointLight::get_static_flags()
{
  return EntityStaticFlags::save_load;
}

//==============================================================================
void PointLight::init
(
  const vec3&   in_position,
  f32           in_radius,
  f32           in_intensity,
  f32           in_falloff,
  const color3& in_color
)
{
  Entity::init(in_position, in_radius);
  this->color     = in_color;
  this->radius    = in_radius;
  this->intensity = in_intensity;
  this->falloff   = in_falloff;
  if (in_radius > 16.0f)
  {
    nc_warn("Light radius is too large. It could lead to bad performance. Consider tweaking light parameters.");
  }
}

//==============================================================================
PointLightGPU PointLight::get_gpu_data(const vec3& position, const vec3& stitched_position, u32 sector_id, f32 radius_mod) const
{
  return PointLightGPU
  {
    .position          = position,
    .intensity         = intensity,
    .stitched_position = stitched_position,
    .radius            = radius * radius_mod,
    .color             = color,
    .falloff           = falloff,
    .sector_id         = sector_id,
  };
}

//==============================================================================
void PointLight::refresh_entity_radius()
{
  this->set_radius(radius);
}

}
