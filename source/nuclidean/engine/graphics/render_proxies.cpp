// Project Nuclidean Source File
#include <engine/graphics/render_proxies.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/render_proxy_registry.h>

#include <common.h>
#include <math/utils.h>

#include <cstring> // strlen
#include <cmath>   // fmod

namespace nc
{

//==============================================================================
void RenderElementCollector::push_back(const RenderElementBillboard& element)
{
  billboards.push_back(element);
}

//==============================================================================
void RenderElementCollector::push_back(const RenderElementLight& element)
{
  lights.push_back(element);
}

//==============================================================================
void RenderElementCollector::push_back(const RenderElementScreenSprite& element)
{
  screen_sprites.push_back(element);
}

//==============================================================================
void RenderElementCollector::push_back(const RenderElementSkyBox& element)
{
  sky_boxes.push_back(element);
}

//==============================================================================
void RenderElementCollector::push_back(const RenderElementAmbientLight& element)
{
  ambient_lights.push_back(element);
}

//==============================================================================
void RenderElementCollector::clear()
{
  screen_sprites.clear();
  billboards.clear();
  lights.clear();
  sky_boxes.clear();
  ambient_lights.clear();
}

//==============================================================================
EntityID IEntityRenderProxy::get_entity_id() const
{
  return m_entity_id;
}

//==============================================================================
f32 IEntityRenderProxy::get_radius() const
{
  return m_radius;
}

//==============================================================================
f32 IEntityRenderProxy::get_height() const
{
  return m_height;
}

//==============================================================================
vec3 IEntityRenderProxy::get_position() const
{
  return m_position;
}

//==============================================================================
void IEntityRenderProxy::set_pos_rad_height(vec3 pos, f32 radius, f32 height)
{
  m_position     = pos;
  m_radius       = radius;
  m_height       = height;
  m_bounds_dirty = true;
}

//==============================================================================
bool IEntityRenderProxy::has_camera_data() const
{
  return false;
}

//==============================================================================
const Camera* IEntityRenderProxy::get_camera_data() const
{
  return nullptr;
}

//==============================================================================
bool IEntityRenderProxy::is_persistent() const
{
  return false;
}

//==============================================================================
BillboardRenderProxy::BillboardRenderProxy(const Appearance& appear)
: appearance(appear)
{
}

//==============================================================================
void BillboardRenderProxy::get_render_elements(RenderElementCollector& collector) const
{
  if (!appearance.sprite)
  {
    return;
  }

  collector.push_back(RenderElementBillboard
  {
    .offset     = VEC3_ZERO,
    .appearance = appearance,
  });
}

//==============================================================================
PlayerRenderProxy::PlayerRenderProxy(const Appearance& appear, const Camera& cam)
: BillboardRenderProxy(appear)
, camera(cam)
{
}

//==============================================================================
void PlayerRenderProxy::get_render_elements(RenderElementCollector& collector) const
{
  BillboardRenderProxy::get_render_elements(collector);

  if (weapon_sprite.sprite)
  {
    collector.push_back(weapon_sprite);
  }
}

//==============================================================================
bool PlayerRenderProxy::has_camera_data() const
{
  return true;
}

//==============================================================================
const Camera* PlayerRenderProxy::get_camera_data() const
{
  return &camera;
}

//==============================================================================
bool PlayerRenderProxy::is_persistent() const
{
  return true;
}

//==============================================================================
void PointLightRenderProxy::get_render_elements(RenderElementCollector& collector) const
{
  collector.push_back(RenderElementLight
  {
    .offset    = VEC3_ZERO,
    .color     = color,
    .intensity = intensity,
    .radius    = radius * this->calc_radius_mod(),
    .falloff   = falloff,
  });
}

//==============================================================================
f32 PointLightRenderProxy::calc_radius_mod() const
{
  f32 cycle_len = intensity_cycle_len;
  if (cycle_len <= 0.0f)
  {
    return 1.0f;
  }

  auto string = intensity_string.to_cstring();
  u64 len = strlen(&string[0]);
  if (len <= 1)
  {
    return 1.0f;
  }

  f64 level_time = GraphicsSystem::get().get_render_proxies().get_level_stats().level_time;
  f64 time = level_time + cast<f64>(intensity_cycle_offset);

  f32 fraction = cast<f32>(fmod(time, cast<f64>(cycle_len)));
  u32 string_idx = min(cast<u32>(len * fraction / cycle_len), cast<u32>(len-1));
  nc_assert(string_idx < len);

  char character = clamp(string[string_idx], 'a', 'z'); // a = 100% intensity, z = 0% intensity
  return 1.0f - (character - 'a') / cast<f32>('z' - 'a');
}

//==============================================================================
void SkyBoxRenderProxy::get_render_elements(RenderElementCollector& collector) const
{
  collector.push_back(sky_box);
}

//==============================================================================
bool SkyBoxRenderProxy::is_persistent() const
{
  return true;
}

//==============================================================================
void AmbientLightRenderProxy::get_render_elements(RenderElementCollector& collector) const
{
  collector.push_back(ambient_light);
}

//==============================================================================
bool AmbientLightRenderProxy::is_persistent() const
{
  return true;
}

}
