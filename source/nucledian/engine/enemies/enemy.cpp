#include <engine/enemies/enemy.h>
#include <engine/core/engine.h>
#include <engine/player/thing_system.h>
#include <engine/player/player.h>
#include <iostream>
#include <engine/map/map_system.h>

#include <math/lingebra.h>

namespace nc
{
  Enemy::Enemy()
  {
    this->position = vec3(0, 0, 0);
    init();

    collision = true;
    width = 0.15f;
    height = 0.35f;

    facing = vec3(0, 0, 1);
    velocity = vec3(0, 0, 0);

    maxHealth = 60;
    health = maxHealth;
  }

  Enemy::Enemy(vec3 position)
  {
    this->position = position;
    init();

    collision = true;
    width = 0.15f;
    height = 0.35f;
    facing = vec3(0, 0, 1);
    velocity = vec3(0, 0, 0);

    maxHealth = 60;
    health = maxHealth;
  }

  Enemy::Enemy(vec3 position, vec3 facing)
  {
    this->position = position;
    init();

    collision = true;
    width = 0.15f;
    height = 0.35f;
    this->facing = normalize(facing);
    velocity = vec3(0, 0, 0);

    maxHealth = 60;
    health = maxHealth;
  }

  void Enemy::init()
  {
    m_entity_index = create_entity(
      Transform(position, vec3(0.3f, 0.7f, 0.3f)),
      colors::RED,
      Transform(vec3(0.0f, 0.35f, 0.0f))
    );
  }

  void Enemy::update()
  {
    vec3 target_pos = get_engine().get_module<ThingSystem>().get_player()->get_position();
    vec3 target_dir = normalize_or_zero(target_pos - position);

    position += target_dir * 1.0f * 0.001f;

    g_transform_components[m_entity_index].position() = position;
  }

  void Enemy::get_wish_velocity(f32 delta_seconds)
  {
    if (!alive)
    {
      velocity = vec3(0, 0, 0);
      return;
    }

    vec3 target_pos = get_engine().get_module<ThingSystem>().get_player()->get_position();
    vec3 target_dir = normalize_or_zero(target_pos - position);

    switch (state)
    {
    case nc::idle:
      velocity = vec3(0, 0, 0);
      if (distance(target_pos, position) <= 1)
      {
        state = chase;
        break;
      }

      if (dot(target_dir, facing) >= cosf(3.14f / 4.0f))
      {
        // RAYCAST CHECK FOR LOS
        [[maybe_unused]] vec3 rayStart = position + vec3(0, 0.5f, 0);
        [[maybe_unused]] vec3 rayEnd = target_pos + vec3(0, 0.5f, 0);

        f32 wallDist;
        vec3 wallNormal;
        const auto& map = get_engine().get_map();
        [[maybe_unused]] bool wallHit = map.raycast3d(rayStart, rayEnd, wallNormal, wallDist);

        if (wallDist < 1.0f)
        {
          break;
        }

        state = chase;
        break;
      }

      break;
    case nc::chase:
      timeRemaining -= delta_seconds;

      velocity = target_dir * 1.0f * delta_seconds;
      break;
    case nc::attack:
      break;
    case nc::hurt:
      break;
    case nc::dead:
      break;
    case nc::count:
      break;
    default:
      break;
    }

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
    MapObject::move(position, velocity, facing, 0.25f);
    g_transform_components[m_entity_index].position() = position;
  }

  void Enemy::damage(int damage)
  {
    health -= damage;
    if (health <= 0)
    {
      die();
    }
  }

  void Enemy::die()
  {
    alive = false;
    collision = false;

    g_appearance_components[m_entity_index].color = colors::GREEN;
    g_appearance_components[m_entity_index].transform = Transform(position, vec3(0.3f, 0.1f, 0.3f));
  }

  bool Enemy::can_attack()
  {
    if (timeRemaining <= 0.0f)
    {
      timeRemaining = attackDelay;
      return true;
    }
    return false;
  }



}


