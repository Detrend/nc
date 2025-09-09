// Project Nucledian Source File
#include <common.h>

#include <engine/enemies/enemy.h>
#include <engine/core/engine.h>
#include <engine/player/thing_system.h>
#include <engine/player/player.h>
#include <engine/map/map_system.h>
#include <engine/map/physics.h>
#include <engine/entity/entity_system.h>

#include <math/lingebra.h>
#include <math/utils.h>

#include <engine/graphics/resources/texture.h>

#include <engine/entity/entity_type_definitions.h>

#include <map>

#ifdef NC_DEBUG_DRAW
#include <engine/graphics/gizmo.h>
#endif

namespace nc
{

//==============================================================================
EntityType Enemy::get_type_static()
{
  return EntityTypes::enemy;
}

//==============================================================================
Enemy::Enemy(vec3 position, vec3 facing)
  : Entity(position, 0.15f, 0.35f, true)
{
  appear = Appearance
  {
    .texture = TextureManager::get()["mff_pepe_walk"],
    .scale = 1.0f,
  };

  collision = true;
  this->facing = normalize(facing);
  velocity = vec3(0, 0, 0);

  maxHealth = 60;
  health = maxHealth;
}

//==============================================================================
void Enemy::update(f32 delta)
{
  if (this->state == EnemyState::dead)
  {
    this->velocity = vec3{0, 0, 0};
    return;
  }

  this->handle_ai(delta);
  this->handle_attack();
  this->handle_movement(delta);
}

//==============================================================================
void Enemy::handle_ai(f32 delta_seconds)
{
  switch (this->state)
  {
    case EnemyState::idle:
    {
      this->handle_idle(delta_seconds);
      break;
    }

    case EnemyState::alert:
    {
      this->handle_alert(delta_seconds);
      break;
    }

    default: break;
  }
}

//==============================================================================
void Enemy::handle_attack()
{
  ThingSystem& game = ThingSystem::get();

  Entity*   target = game.get_entities().get_entity(this->target_id);
  PhysLevel lvl    = game.get_level();

  if (this->can_attack() && target)
  {
    time_until_attack = attackDelay;

    vec3 ray_start = this->get_position()   + vec3(0, 0.5f, 0);
    vec3 ray_end   = target->get_position() + vec3(0, 0.5f, 0);

    auto wall_hit = lvl.ray_cast_3d(ray_start, ray_end);
    if (wall_hit) 
    {
      return;
    }

    if (Player* player = target->as<Player>())
    {
      player->damage(10);
    }
  }
}

//==============================================================================
void Enemy::handle_movement(f32 delta)
{
  auto world = ThingSystem::get().get_level();

  vec3 position = this->get_position();
  world.move_character
  (
    position, this->velocity, &this->facing, delta, 0.25f,
    0.5f, 0.0f, PhysLevel::COLLIDE_ALL, 0
  );
  this->set_position(position);
}

//==============================================================================
void Enemy::damage(int damage)
{
  health -= damage;
  if (health <= 0)
  {
    //this->die();
  }
}

//==============================================================================
void Enemy::die()
{
  this->collision = false;
}

//==============================================================================
void Enemy::handle_idle(f32 /*delta*/)
{
  auto& game = ThingSystem::get();

  Player* player = game.get_player();
  this->velocity = vec3{0, 0, 0};

  if (!player)
  {
    return;
  }

  vec3 player_pos = game.get_player()->get_position();

  if (distance(player->get_position(), this->get_position()) <= 1.0f)
  {
    this->state          = EnemyState::alert;
    this->target_id      = player->get_id();
    this->can_see_target = true;
    this->time_since_saw_target = 0.0f;
    return;
  }

  vec3 player_dir = normalize(player->get_position() - this->get_position());

  if (dot(player_dir, this->facing) >= std::cosf(PI / 4.0f))
  {
    vec3 ray_end = player_pos + vec3(0, 0.5f, 0);

    if (this->can_see_point(ray_end))
    {
      this->state          = EnemyState::alert;
      this->target_id      = player->get_id();
      this->can_see_target = true;
      this->time_since_saw_target = 0.0f;
    }
  }
}

//==============================================================================
void Enemy::handle_alert(f32 delta)
{
  auto& engine = get_engine();
  auto& game   = ThingSystem::get();
  auto& ecs    = game.get_entities();
  auto& map    = game.get_map();

  Entity* target = ecs.get_entity(this->target_id);
  if (!target)
  {
    // Return to idle state if the target is dead or does not exist
    this->state     = EnemyState::idle;
    this->target_id = INVALID_ENTITY_ID;
    this->can_see_target = false;
    this->time_since_saw_target = 0.0f;
    this->current_path.points.clear();
    return;
  }

  // Handle target visibility once in a 16 frames
  u64 frame_idx = engine.get_frame_idx();
  u64 my_idx    = this->get_id().idx;
  if ((frame_idx % 16) == (my_idx % 16))
  {
    vec3 pt = target->get_position() + vec3{0, 0.5f, 0};
    this->can_see_target = this->can_see_point(pt);
  }

  // Update the last seen pos if we see the target
  if (this->can_see_target)
  {
    this->follow_target_pos = target->get_position();
    this->time_since_saw_target = 0.0f;
  }
  else
  {
    this->time_since_saw_target += delta;
  }

  // Recompute the path if needed
  bool no_path  = current_path.points.empty();
  vec2 last_pt2 = no_path ? VEC2_ZERO : current_path.points.back().xz();
  if (no_path || distance(last_pt2, this->follow_target_pos.xz()) > 5.0f)
  {
    this->current_path.points = map.get_path
    (
      this->get_position(),
      this->follow_target_pos,
      this->get_radius(),
      this->get_height()
    );
  }

  // Handle attack
  time_until_attack -= delta;
  if (this->can_see_target)
  {
    this->handle_attack();
  }

  auto& path = this->current_path.points;
  vec2  pos2 = this->get_position().xz();

  // Erase the first point of the path if we are too close
  while (path.size() && distance(pos2, path[0].xz()) < 0.25f)
  {
    path.erase(path.begin());
  }

  if (path.size())
  {
    vec2 target_dir = normalize_or_zero(path[0].xz() - pos2);
    if (target_dir != VEC2_ZERO)
    {
      this->facing = vec3{target_dir.x, 0.0f, target_dir.y};
    }

    if (this->can_see_target && distance(this->follow_target_pos.xz(), pos2) < 3.0f)
    {
      this->velocity = VEC3_ZERO;
    }
    else
    {
      this->velocity = vec3{target_dir.x, 0.0f, target_dir.y};
    }
  }
  else
  {
    this->velocity = VEC3_ZERO;
  }

  // Handle gravity
  velocity.y -= GRAVITY * delta;

  // Debug draw points
#ifdef NC_DEBUG_DRAW
  {
    vec3 pos_2D = this->get_position();
    pos_2D.y = 0;

    for (u64 i = 0; i < path.size(); ++i)
    {
      vec2 p1 = i ? path[i-1].xz() : pos_2D.xz();
      vec2 p2 = path[i].xz();
      Gizmo::create_line_2d("Paths", p1, p2, colors::BLUE);
    }
  }
#endif
}

//==============================================================================
bool Enemy::can_see_point(vec3 pt) const
{
  constexpr EntityTypeMask COLLS = PhysLevel::COLLIDE_NONE;
  auto lvl = ThingSystem::get().get_level();

  vec3 from = this->get_position() + vec3{0, 0.5f, 0};
  vec3 to   = pt;

  CollisionHit hit = lvl.ray_cast_3d(from, to, COLLS);

  return hit ? false : true;
}

//==============================================================================
bool Enemy::can_attack() const
{
  return this->time_until_attack <= 0.0f;
}

//==============================================================================
vec3& Enemy::get_velocity()
{
  return this->velocity;
}

//==============================================================================
const Appearance& Enemy::get_appearance() const
{
  return this->appear;
}

//==============================================================================
Appearance& Enemy::get_appearance()
{
  return this->appear;
}

}
