#include <engine/graphics/lights.h>

#include <common.h>

#include <engine/entity/entity_type_definitions.h>

namespace nc
{

//==============================================================================
EntityType DirectionalLight::get_type_static()
{
  return EntityTypes::directional_light;
}

//==============================================================================
DirectionalLight::DirectionalLight(const vec3& direction, f32 intensity, const color3& color)
  : Entity(VEC3_ZERO, 0.0f, 0.0f), color(color), direction(direction), intensity(intensity) {}

//==============================================================================
DirectionalLight::GPUData DirectionalLight::get_gpu_data() const
{
  return GPUData
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
f32 PointLight::calculate_radius() const
{
  // TODO: we should probably also consider light intensity

  constexpr f32 threshold = 5.0f / 255.0f;

  const f32 c = constant - (1.0f / threshold);
  const f32 d = (linear * linear) - (4.0f * quadratic * c);

  nc_assert(d >= 0.0f);

  return (-linear + sqrtf(d)) / (2.0f * quadratic);
}

}
