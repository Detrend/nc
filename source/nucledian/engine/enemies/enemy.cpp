// Project Nucledian Source File
#include <common.h>

#include <engine/enemies/enemy.h>
#include <engine/core/engine.h>
#include <engine/player/thing_system.h>
#include <engine/player/player.h>
#include <engine/map/map_system.h>
#include <engine/map/physics.h>
#include <engine/entity/entity_system.h>

#include <game/weapons.h>
#include <game/projectile.h>

#include <math/lingebra.h>
#include <math/utils.h>

#include <cstdlib> // std::rand

#include <engine/graphics/resources/texture.h>

#include <engine/entity/entity_type_definitions.h>
#include <profiling.h>

#include <map>
#include <format>

#ifdef NC_DEBUG_DRAW
#include <engine/graphics/gizmo.h>
#endif


namespace nc
{

constexpr f32 SPOT_DISTANCE         = 1.0f;  // Player will get spotted if closer
constexpr f32 ENEMY_FOV_DEG         = 45.0f; // Field of view
constexpr f32 TARGET_STOP_DISTANCE  = 1.0f;  // How far do we keep from the target
constexpr f32 PATH_POINT_ERASE_DIST = 0.25f; // Removes the path point if this close
constexpr f32 ATTACK_DELAY_MIN      = 3.0f;
constexpr f32 ATTACK_DELAY_MAX      = 8.0f;

constexpr f32 ENEMY_HEIGHT = 2.5f;
constexpr f32 ENEMY_RADIUS = 1.0f;

//==============================================================================
static f32 random_range(f32 min, f32 max)
{
  NC_TODO("Implement a proper system for generating random numbers.");

  f32 dist = max - min;
  nc_assert(dist >= 0.0f);

  f32 coeff = (std::rand() % 1024) / cast<f32>(1023);
  return min + dist * coeff;
}

//==============================================================================
EntityType Enemy::get_type_static()
{
  return EntityTypes::enemy;
}

//==============================================================================
Enemy::Enemy(vec3 position, vec3 looking_dir)
 : Entity(position, ENEMY_RADIUS, ENEMY_HEIGHT, true)
 , facing(looking_dir)
 , anim_fsm(AnimStates::idle)
{
  // TODO: Move somewhere else
  constexpr int MAX_HEALTH = 60;
  nc_assert(looking_dir != VEC3_ZERO);

  this->facing    = normalize(this->facing);
  this->collision = true;
  this->velocity  = VEC3_ZERO;
  this->health    = MAX_HEALTH;

  this->appear = Appearance
  {
    .sprite    = "cultist_idle_0",
    .direction = this->facing,
    .scale     = 45.0f,
    .mode      = Appearance::SpriteMode::dir8,
    .pivot     = Appearance::PivotMode::bottom,
  };

  this->anim_fsm.set_state_length(AnimStates::idle,   1.0f);
  this->anim_fsm.set_state_length(AnimStates::walk,   1.25f);
  this->anim_fsm.set_state_length(AnimStates::attack, 3.75f);
  this->anim_fsm.set_state_length(AnimStates::dead,   2.0f);
  this->anim_fsm.add_trigger(AnimStates::attack, 2.3f, TriggerTypes::trigger_fire);
}

//==============================================================================
void Enemy::update(f32 delta)
{
  NC_SCOPE_PROFILER(EnemyUpdate)

  if (this->state == EnemyAiState::dead)
  {
    this->velocity = VEC3_ZERO;
    return;
  }

  this->handle_ai(delta);
  this->handle_movement(delta);
  this->handle_appearance(delta);
}

//==============================================================================
void Enemy::handle_ai(f32 delta_seconds)
{
  switch (this->state)
  {
    case EnemyAiState::idle:
    {
      this->handle_ai_idle(delta_seconds);
      break;
    }

    case EnemyAiState::alert:
    {
      this->handle_ai_alert(delta_seconds);
      break;
    }

    default: break;
  }
}

//==============================================================================
void Enemy::handle_movement(f32 delta)
{
  auto world = ThingSystem::get().get_level();

  // Handle gravity
  velocity.y -= GRAVITY * delta;

  vec3 position = this->get_position();
  mat4 portal_transform = identity<mat4>();
  world.move_character
  (
    position, this->velocity, portal_transform, delta, ENEMY_HEIGHT,
    ENEMY_RADIUS, ENEMY_HEIGHT * 0.3f, PhysLevel::COLLIDE_ALL, 0
  );
  this->set_position(position);
}

//==============================================================================
void Enemy::handle_appearance(f32 delta)
{
  this->appear.direction = this->facing;

  using Event   = AnimFSMEvents::evalue;
  using Trigger = EnemyFSM::Trigger;
  using State   = EnemyFSM::State;

  this->anim_fsm.update(delta, [&](Event etype, Trigger ttype, State)
  {
    // Do nothing here
    if (etype == Event::trigger && ttype == TriggerTypes::trigger_fire)
    {
      vec3 dir  = this->facing;
      vec3 from = this->get_position() + UP_DIR * ENEMY_HEIGHT * 0.7f + dir * 0.3f;

      // Fire!
      auto& game = ThingSystem::get();
      game.get_entities().create_entity<Projectile>
      (
        from, dir, this->get_id(), WeaponTypes::plasma_rifle
      );
    }
  });

  // Calculate the correct sprite
  constexpr u64 SPRITE_CNTS[]
  {
    1,
    16,
    45,
    1,
  };
  static_assert(ARRAY_LENGTH(SPRITE_CNTS) == AnimStates::count);

  u8   anim_state       = this->anim_fsm.get_state();
  u64  state_sprite_cnt = SPRITE_CNTS[anim_state];
  f32  rel_time         = this->anim_fsm.get_time_relative();
  u64  sprite_idx       = cast<u64>(rel_time * state_sprite_cnt);
  cstr sprite_sheet     = ANIM_STATES_NAMES[anim_state];

  this->appear.sprite = std::format("cultist_{}_{}", sprite_sheet, sprite_idx);
}

//==============================================================================
void Enemy::damage(int damage, EntityID from_who)
{
  auto& game = ThingSystem::get();
  Entity* attacker = game.get_entities().get_entity(from_who);

  if (attacker)
  {
    this->state = EnemyAiState::alert;
    this->target_id = from_who;
    this->can_see_target = false;
    this->time_since_saw_target = 0.0f;
    this->follow_target_pos = attacker->get_position();
  }

  health -= damage;
  if (health <= 0)
  {
    this->die();
  }
}

//==============================================================================
void Enemy::die()
{
  this->collision = false;
  this->state = EnemyAiState::dead;
  ThingSystem::get().get_entities().destroy_entity(this->get_id());
}

//==============================================================================
void Enemy::handle_ai_idle(f32 /*delta*/)
{
  auto& game = ThingSystem::get();

  Player* player = game.get_player();
  this->velocity = VEC3_ZERO;

  anim_fsm.require_state(AnimStates::idle);

  if (!player)
  {
    return;
  }

  vec3 player_pos = player->get_position();
  vec3 player_dir = normalize_or_zero(player_pos - this->get_position());

  bool transition_to_alert = false;

  if (distance(player->get_position(), this->get_position()) <= SPOT_DISTANCE)
  {
    transition_to_alert = true;
  }
  else if (dot(player_dir, this->facing) >= cos(rad2deg(ENEMY_FOV_DEG)))
  {
    vec3 ray_end = player_pos + vec3(0, 0.5f, 0);

    if (this->can_see_point(ray_end))
    {
      transition_to_alert = true;
    }
  }

  if (transition_to_alert)
  {
    this->state          = EnemyAiState::alert;
    this->target_id      = player->get_id();
    this->can_see_target = true;
    this->time_since_saw_target = 0.0f;
  }
}

//==============================================================================
void Enemy::handle_ai_alert(f32 delta)
{
  auto& engine = get_engine();
  auto& game   = ThingSystem::get();
  auto& ecs    = game.get_entities();
  auto& map    = game.get_map();
  auto  lvl    = game.get_level();

  Entity* target = ecs.get_entity(this->target_id);
  if (!target)
  {
    // Return to idle state if the target is dead or does not exist
    this->state     = EnemyAiState::idle;
    this->target_id = INVALID_ENTITY_ID;
    this->can_see_target = false;
    this->time_since_saw_target = 0.0f;
    this->current_path.points.clear();
    return;
  }

  vec2 position_2d = this->get_position().xz();

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

  // Animation
  switch (this->anim_fsm.get_state())
  {
    // STANDING or WALKING
    case AnimStates::idle: [[fallthrough]];
    case AnimStates::walk:
    {
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

        lvl.smooth_out_path
        (
          this->current_path.points, this->get_radius(), this->get_height()
        );
      }

      // Cooldown after attack
      time_until_attack -= delta;

      vec2 move_path_force        = VEC2_ZERO; // Moving towards the target
      vec2 move_from_target_force = VEC2_ZERO; // Moving from the target if too close
      vec2 move_from_mates_force  = VEC2_ZERO; // Moving away from other enemies if too close

      auto& path = this->current_path.points;

      // Erase the first point of the path if we are too close
      while (path.size() && distance(position_2d, path[0].xz()) < PATH_POINT_ERASE_DIST)
      {
        path.erase(path.begin());
      }

      // Compute force moving as towards the closest point of the path
      if (path.size())
      {
        move_path_force = normalize_or_zero(path[0].xz() - position_2d);
      }

      // Compute force for moving away from the target (so we do not get
      // too close)
      vec2 target_dir  = target->get_position().xz() - position_2d;
      f32  target_dist = length(target_dir);
      if (target_dist < TARGET_STOP_DISTANCE)
      {
        f32 coeff = 1.0f - target_dist / TARGET_STOP_DISTANCE;
        move_from_target_force = -normalize_or_zero(target_dir) * coeff * 4.0f;
      }

      vec2 final_force = VEC2_ZERO;
      final_force += move_path_force;
      final_force += move_from_target_force;
      final_force += move_from_mates_force;
      final_force = clamp_length(final_force, 0.0f, 1.0f);

      this->velocity.x = final_force.x;
      this->velocity.z = final_force.y;

      bool is_moving = length(this->velocity.xz()) > 0.1f;
      AnimStates desired_state = AnimStates::idle;

      if (is_moving)
      {
        this->facing = normalize(vec3{final_force.x, 0.0f, final_force.y});
        desired_state = AnimStates::walk;
      }

      if (this->can_attack() && this->can_see_target)
      {
        desired_state = AnimStates::attack;
        time_until_attack = random_range(ATTACK_DELAY_MIN, ATTACK_DELAY_MAX);
      }

      if (this->anim_fsm.get_state() != desired_state)
      {
        this->anim_fsm.set_state(desired_state);
      }

      break;
    }

    // ATTACKING
    case AnimStates::attack:
    {
      this->velocity.x = this->velocity.z = 0.0f; // Stand on a spot
      vec2 dir_to_target = normalize_or_zero(this->follow_target_pos.xz() - position_2d);
      if (dir_to_target != VEC2_ZERO)
      {
        this->facing = vec3{dir_to_target.x, 0.0f, dir_to_target.y};
      }
      break;
    }
  }

  // Debug draw path points
#ifdef NC_DEBUG_DRAW
  {
    for (u64 i = 0; i < current_path.points.size(); ++i)
    {
      vec2 p1 = i ? current_path.points[i-1].xz() : position_2d;
      vec2 p2 = current_path.points[i].xz();
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
  bool cooldown_ok = this->time_until_attack <= 0.0f;
  bool state_ok    = anim_fsm.get_state() != AnimStates::attack;
  return cooldown_ok && state_ok;
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
