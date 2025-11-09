#include <engine/graphics/lights.h>

#include <common.h>

#include <engine/entity/entity_type_definitions.h>

namespace nc
{

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
  const vec3& position,
  f32 intensity,
  f32 constant,
  f32 linear,
  f32 quadratic,
  const color3& color
)
:
  Entity(position, calculate_radius(color, intensity, constant, linear, quadratic)),
  color(color),
  intensity(clamp(intensity, 0.0f, 1.0f)),
  constant(constant),
  linear(linear),
  quadratic(quadratic)
{
  if (intensity < 0.0f || intensity > 1.0f)
  {
    nc_warn("Light intensity is clamped because it is outside [0, 1] range.");
  }

  const f32 radius = get_radius();
  if (radius > 16.0f)
  {
    nc_warn("Light radius is too large. It could lead to bad performance. Consider tweaking light parameters.");
  }
}

//==============================================================================
PointLightGPU PointLight::get_gpu_data(const vec3& position) const
{
  return PointLightGPU
  {
    .position = position,
    .intensity = intensity,
    .color = color,
    .constant = constant,
    .linear = linear,
    .quadratic = quadratic,
  };
}

//==============================================================================
void PointLight::refresh_entity_radius()
{
  f32 radius = calculate_radius
  (
    this->color, this->intensity, this->constant, this->linear, this->quadratic
  );

  this->set_radius(max(radius, 0.0f));
}

//==============================================================================
f32 PointLight::calculate_radius(const color3& color, f32 intensity, f32 constant, f32 linear, f32 quadratic)
{
  // WARNING: Following code should be 1:1 copy of the calculate_light_radius function in light_culling.comp shader.

  const float max_color = max(color.r, max(color.g, color.b));
  const float min_intensity = 32.0f;
  const float k = max_color * intensity * min_intensity;

  if (quadratic < 0.0001f)
    return (k - constant) / linear;

  const float discriminant = linear * linear - 4.0f * quadratic * (constant - k);
  if (discriminant < 0.0f)
    return 0.0f;

  return (-linear + sqrt(discriminant)) / (2.0f * quadratic);
}

}
