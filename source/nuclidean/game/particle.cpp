// Project Nuclidean Source File
#include <game/particle.h>

#include <engine/entity/entity_type_definitions.h>

#include <engine/game/game_system.h>
#include <engine/entity/entity_system.h>
#include <engine/graphics/entities/lights.h>
#include <game/entity_attachment_manager.h>

#include <format>

namespace nc
{

//==============================================================================
/*static*/ EntityType Particle::get_type_static()
{
  return EntityTypes::particle;
}

//==============================================================================
void Particle::init
(
  vec3   position,
  cstr   sprite,
  u32    num_imgs,
  f32    duration,
  color3 light,
  f32    light_range,
  f32    scale
)
{
  nc_assert(light_range >= 0.0f);
  nc_assert(duration    >= 0.0f);

  Entity::init(position, light_range);
  this->m_light_color   = light;
  this->m_img_cnt       = sprite ? num_imgs : 0;
  this->m_duration      = duration;
  this->m_lifetime      = 0.0f;
  this->m_initial_range = light_range;

  if (sprite)
  {
    if (m_img_cnt > 1)
    {
      m_appear.sprite = std::format("{}_{}", sprite, 0);
    }
    else
    {
      m_appear.sprite = sprite;
    }

    m_appear.scale    = scale;
    m_appear.mode     = Appearance::SpriteMode::mono;
    m_appear.pivot    = Appearance::PivotMode::centered;
    //m_appear.scaling  = Appearance::ScalingMode::fixed;
    m_appear.rotation = Appearance::RotationMode::full;
  }
}

//==============================================================================
void Particle::init
(
  vec3   position,
  f32    duration,
  color3 light,
  f32    light_range,
  f32    scale
)
{
  this->init(position, nullptr, 0, duration, light, light_range, scale);
}

//==============================================================================
void Particle::post_init()
{
  if (m_initial_range > 0.0f)
  {
    // Spawn a light as well
    EntityAttachment& attachment_mgr = GameSystem::get().get_attachment_mgr();
    EntityRegistry&   ecs            = GameSystem::get().get_entities();

    // Is this valid?
    vec3 position = this->get_position();

    // Spawn the light
    PointLight* light = ecs.create_entity<PointLight>
    (
      position, m_initial_range, 1.0f, 1.0f, m_light_color
    );

    nc_assert(light);

    // And attach it
    attachment_mgr.attach_entity(light->get_id(), this->get_id());
  }
}

//==============================================================================
void Particle::update(f32 delta)
{
  m_lifetime += delta;

  EntityRegistry& ecs = GameSystem::get().get_entities();

  if (m_lifetime >= m_duration)
  {
    // Kill ourselves
    ecs.destroy_entity(this->get_id());
    return;
  }

  std::string as_string = m_appear.sprite.to_string();

  // Update the sprite anim
  if (as_string.length() && m_img_cnt > 1)
  {
    // Remove everything after the last "_"
    while (as_string.back() != '_')
    {
      as_string.pop_back();
      nc_assert(as_string.length());
    }

    // Remove the "_" as well
    as_string.pop_back();

    // Calculate the index
    f32 frac = m_lifetime / m_duration;
    u32 idx  = cast<u32>(frac * m_img_cnt);
    m_appear.sprite = std::format("{}_{}", as_string, idx);
  }

  // Update the light
  if (m_light_opt != INVALID_ENTITY_ID)
  {
    PointLight* light = ecs.get_entity<PointLight>(m_light_opt);
    nc_assert(light);

    // Calculate new radius
    f32 frac = m_lifetime / m_duration;
    f32 new_radius = m_initial_range * (1.0f - frac);
    nc_assert(light->get_radius() >= new_radius);

    light->radius = new_radius;
    light->refresh_entity_radius();
  }
}

//==============================================================================
Appearance* Particle::get_appearance()
{
  return m_img_cnt ? &m_appear : nullptr;
}

//==============================================================================
const Appearance* Particle::get_appearance() const
{
  return const_cast<Particle*>(this)->get_appearance();
}

}
