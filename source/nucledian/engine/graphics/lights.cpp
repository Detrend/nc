#include <engine/graphics/lights.h>

#include <common.h>

#include <engine/entity/entity_type_definitions.h>

namespace nc
{

//==============================================================================
// Stolen from light_culling.comp
static f32 calculate_point_light_radius
(
  color3 color, f32 intensity, f32 constant, f32 linear, f32 quadratic
)
{
  f32 max_color = max(color.r, max(color.g, color.b));
  f32 min_intensity = 64.0f;
  f32 k = max_color * intensity * min_intensity;

  if (quadratic < 0.0001)
    return (k - constant) / linear;

  f32 discriminant = linear * linear - 4 * quadratic * (constant - k);
  if (discriminant < 0)
    return 0;

  return (-linear + sqrtf(discriminant)) / (2.0f * quadratic);
}

//==============================================================================
EntityType DirectionalLight::get_type_static()
{
  return EntityTypes::directional_light;
}

//==============================================================================
DirectionalLight::DirectionalLight(const vec3& direction, f32 intensity, const color3& color)
  : Entity(VEC3_ZERO, 0.0f, 0.0f), color(color), direction(direction), intensity(intensity) {}

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
: Entity
  (
    position,
    min(calculate_point_light_radius(color, intensity, constant, linear, quadratic), 15.0f),
    0.0f
  )
, color(color)
, intensity(intensity)
, constant(constant)
, linear(linear)
, quadratic(quadratic)
{
  // refresh_entity_radius() can't be called here for now as there is a bug in
  // the sector system due to which the entity might be initialized sooner than
  // the sector system, which leads to a null pointer dereference :(
}

//==============================================================================
void PointLight::refresh_entity_radius()
{
  f32 radius = calculate_point_light_radius
  (
    this->color, this->intensity, this->constant, this->linear, this->quadratic
  );

  this->set_radius(clamp(radius, 0.0f, 15.0f));
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
f32 PointLight::calculate_radius() const
{
  // TODO: include light intensity

  constexpr f32 threshold = 5.0f / 255.0f;

  const f32 c = constant - (1.0f / threshold);
  const f32 d = (linear * linear) - (4.0f * quadratic * c);

  nc_assert(d >= 0.0f);

  return (-linear + sqrtf(d)) / (2.0f * quadratic);
}

}
