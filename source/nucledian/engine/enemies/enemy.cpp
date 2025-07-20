#include <engine/enemies/enemy.h>
#include <engine/core/engine.h>
#include <engine/player/thing_system.h>
#include <engine/player/player.h>
#include <iostream>
#include <engine/map/map_system.h>
#include <engine/map/physics.h>

#include <math/lingebra.h>
#include <engine/graphics/resources/texture.h>

#include <engine/entity/entity_type_definitions.h>

namespace nc
{

  //==============================================================================
  EntityType Enemy::get_type_static()
  {
    return EntityTypes::enemy;
  }

  Enemy::Enemy(vec3 position, vec3 facing)
  : Base(position, 0.15f, 0.35f, true)
  {
    init();

    collision = true;
    this->facing = normalize(facing);
    velocity = vec3(0, 0, 0);

    maxHealth = 60;
    health = maxHealth;
  }

  void Enemy::init()
  {
    appear = Appearance
    {
      .texture = TextureManager::instance().get_test_enemy_texture(),
      .scale = 1.0f,
    };
  }

  void Enemy::update()
  {
    vec3 target_pos = get_engine().get_module<ThingSystem>().get_player()->get_position();
    vec3 target_dir = normalize_or_zero(target_pos - this->get_position());

    this->set_position(this->get_position() + target_dir * 1.0f * 0.001f);

    //g_transform_components[m_entity_index].position() = this->get_position();
  }

  void Enemy::get_wish_velocity(f32 delta_seconds)
  {
    if (!alive)
    {
      velocity = vec3(0, 0, 0);
      return;
    }

    vec3 target_pos = get_engine().get_module<ThingSystem>().get_player()->get_position();
    vec3 target_dir = normalize_or_zero(target_pos - this->get_position());

    switch (state)
    {
    case nc::idle:
      velocity = vec3(0, 0, 0);
      if (distance(target_pos, this->get_position()) <= 1)
      {
        state = chase;
        break;
      }

      if (dot(target_dir, facing) >= cosf(3.14f / 4.0f))
      {
        // RAYCAST CHECK FOR LOS
        [[maybe_unused]] vec3 rayStart = this->get_position() + vec3(0, 0.5f, 0);
        [[maybe_unused]] vec3 rayEnd = target_pos + vec3(0, 0.5f, 0);

        auto wallHit = ThingSystem::get().get_level().ray_cast_3d(rayStart, rayEnd);

        if (wallHit)
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

  void Enemy::apply_velocity()
  {
    auto world = ThingSystem::get().get_level();

    vec3 position = this->get_position();
    world.move_character(position, velocity, &facing, 1.0f, 0.25f, 0.5f, 0.0f, 0, 0);
    this->set_position(position);
    //g_transform_components[m_entity_index].position() = position;
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

    //g_appearance_components[m_entity_index].color = colors::GREEN;
    //g_appearance_components[m_entity_index].transform = Transform(this->get_position(), vec3(0.3f, 0.1f, 0.3f));
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

  vec3& Enemy::get_velocity()
  {
    return velocity;
  }



  //==============================================================================
  const Appearance& Enemy::get_appearance() const
  {
    return appear;
  }

  //==============================================================================
  Appearance& Enemy::get_appearance()
  {
    return appear;
  }

  //==============================================================================
  Transform Enemy::calc_transform() const
  {
    return Transform{this->get_position()};
  }

}


