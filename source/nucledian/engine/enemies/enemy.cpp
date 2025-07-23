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

#include <map>

namespace nc
{

  //==============================================================================

  EntityType Enemy::get_type_static()
  {
    return EntityTypes::enemy;
  }

  //==============================================================================

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

  //==============================================================================

  void Enemy::init()
  {
    appear = Appearance
    {
      .texture = TextureManager::instance().get_test_enemy_texture(),
      .scale = 1.0f,
    };
  }

  //==============================================================================

  void Enemy::update()
  {
    vec3 target_pos = get_engine().get_module<ThingSystem>().get_player()->get_position();
    vec3 target_dir = normalize_or_zero(target_pos - this->get_position());

    this->set_position(this->get_position() + target_dir * 1.0f * 0.001f);

    //g_transform_components[m_entity_index].position() = this->get_position();
  }

  //================================================================================

  std::vector<vec3> Enemy::get_path(const MapSectors& map, vec3 start_pos, vec3 end_pos)
  {
    std::vector<vec3> path;
    path.push_back(end_pos);

    struct PrevPoint
    {
      SectorID prev_sector; // previous sector
      WallID wall_index; // portal we used to get to this sector
      vec3 point; // way point
    };

    SectorID startID = map.get_sector_from_point(start_pos.xz);
    SectorID endID = map.get_sector_from_point(end_pos.xz);
    SectorID curID = startID;

    std::vector<SectorID> fringe;
    std::map<SectorID, PrevPoint> visited;

    visited.insert({ startID, {
      INVALID_SECTOR_ID,
      INVALID_WALL_ID,
      vec3(0, 0, 0)
      } });

    while (fringe.size())
    {
      // get sector from queue
      curID = fringe.front();
      fringe.erase(fringe.begin());

      // found path, end search
      if (curID == endID)
      {
        break;
      }
    }

    PrevPoint prev_point;

    // reconstruct the path in reverse order
    while (curID != startID)
    {
      prev_point = visited[curID];

      path.push_back(prev_point.point);

      curID = prev_point.prev_sector;
    }

    // reverse the path
    std::reverse(path.begin(), path.end());

    return path;
  }

  //==============================================================================

  void Enemy::calculate_wish_velocity(f32 delta_seconds)
  {
    if (!alive)
    {
      velocity = vec3(0, 0, 0);
      return;
    }

    vec3 target_pos = get_engine().get_module<ThingSystem>().get_player()->get_position();

    vec3 pos_2D = get_position();
    pos_2D.y = 0;

    const auto& map = get_engine().get_map();
    std::vector<vec3> path = map.get_path(get_position(), target_pos, get_radius(), get_height());

    vec3 target_dir = target_pos - this->get_position();
    target_dir.y = 0;
    target_dir = normalize_or_zero(target_dir);

    switch (state)
    {
    case EnemyState::idle:
      velocity = vec3(0, 0, 0);
      if (distance(target_pos, this->get_position()) <= 1)
      {
        state = EnemyState::chase;
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

        state = EnemyState::chase;
        break;
      }

      break;
    case EnemyState::chase:

      target_dir = path[0] - pos_2D;
      target_dir = normalize_or_zero(target_dir);

      timeRemaining -= delta_seconds;

      velocity = target_dir * 1.0f * delta_seconds;
      velocity.y -= GRAVITY * delta_seconds;



      break;
    case EnemyState::attack:
      break;
    case EnemyState::hurt:
      break;
    case EnemyState::dead:
      break;
    case EnemyState::count:
      break;
    default:
      break;
    }

  }

  //==============================================================================
  void Enemy::apply_velocity()
  {
    auto world = ThingSystem::get().get_level();
    const auto& map = get_engine().get_map();
    f32 floor = 0;
    auto sector_id = map.get_sector_from_point(this->get_position().xz());
    if (sector_id != INVALID_SECTOR_ID)
    {
      const f32 sector_floor_y = map.sectors[sector_id].floor_height;
      floor = sector_floor_y;
      if (this->get_position().y + velocity.y < floor)
      {
        //velocity.y = floor - this->get_position().y;
        velocity.y = 0;
      }
    }

    vec3 position = this->get_position();
    world.move_character(position, velocity, &facing, 1.0f, 0.25f, 0.5f, 0.0f, 0, 0);
    this->set_position(position);
    //g_transform_components[m_entity_index].position() = position;
  }

  //==============================================================================

  void Enemy::damage(int damage)
  {
    health -= damage;
    if (health <= 0)
    {
      die();
    }
  }

  //==============================================================================

  void Enemy::die()
  {
    alive = false;
    collision = false;

    //g_appearance_components[m_entity_index].color = colors::GREEN;
    //g_appearance_components[m_entity_index].transform = Transform(this->get_position(), vec3(0.3f, 0.1f, 0.3f));
  }

  //==============================================================================

  bool Enemy::can_attack()
  {
    if (timeRemaining <= 0.0f)
    {
      timeRemaining = attackDelay;
      return true;
    }
    return false;
  }

  //==============================================================================

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
    return Transform{ this->get_position() };
  }

}


