#include <engine/enemies/enemy.h>
#include <engine/core/engine.h>
#include <engine/player/thing_system.h>
#include <engine/player/player.h>
#include <iostream>

namespace nc 
{
Enemy::Enemy()
{
  position = vec3(0, 0, 0);
  init();

  collision = true;
  width = 0.5f;
  height = 0.5f;
}

Enemy::Enemy(vec3 position)
{
  position = position;
  init();

  collision = true;
  width = 0.5f;
  height = 0.5f;
}

void Enemy::init()
{
  m_entity_index = create_entity(position, 1, colors::GREEN);
}

void Enemy::update()
{
  vec3 target_pos = get_engine().get_module<ThingSystem>().get_player()->get_position();
  vec3 target_dir = normalize_or_zero(target_pos - position);

  position += target_dir * 1.0f * 0.001f;

  m_position_components[m_entity_index] = position;
}

void Enemy::get_wish_velocity(f32 delta_seconds)
{
  vec3 target_pos = get_engine().get_module<ThingSystem>().get_player()->get_position();
  vec3 target_dir = normalize_or_zero(target_pos - position);
  velocity = target_dir * 1.0f * delta_seconds;
}

void Enemy::check_for_collision(MapObject collider)
{
  position += velocity;
 
  if (!did_collide(collider))
  {
    position += -velocity;
    return;
  }

  //vec3 target_pos = collider.get_position();
  vec3 target_dist = collider.get_position() - position;

  vec3 intersect_union = vec3(get_width(), 0, get_width()) - (target_dist - vec3(collider.get_width(), 0, collider.get_width()));
  
  position += -velocity;

  vec3 new_velocity = velocity - intersect_union;
  vec3 mult = vec3(velocity.x / (1 - new_velocity.x), 0, velocity.z / (1 - new_velocity.z));

  if (mult.x < 0.05f) mult.x = 0;
  if (mult.z < 0.05f) mult.z = 0;

  velocity.x = velocity.x * mult.x;
  velocity.z = velocity.z * mult.z;
}

void Enemy::apply_velocity()
{
  position += velocity;
  m_position_components[m_entity_index] = position;
}



}


