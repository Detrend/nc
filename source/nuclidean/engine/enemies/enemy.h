// Project Nuclidean Source File
#pragma once

#include <engine/entity/entity.h>
#include <engine/entity/entity_types.h>

#include <engine/map/map_types.h>
#include <engine/appearance.h>
#include <anim_state_machine.h>

#include <game/game_types.h> // EnemyType
#include <game/enemies.h>
#include <rng.h>
#include <engine/game/game_helpers.h>

namespace nc
{

struct EnemyStats;

class Enemy : public Entity
{
public:
  static EntityType      get_type_static();
  static EntityStatFlags get_static_flags();

  void init(vec3 position, vec3 facing, EnemyType type);
  void post_init();

  void update(f32 delta);

  // Take damage, save inflictor
  void damage(int damage, EntityID from_who);

  Appearance&       get_appearance();
  const Appearance& get_appearance() const;
  const EnemyStats& get_stats()      const;

  vec3 get_attack_from_pos() const;
  vec3 get_eye_pos()         const;
  vec3 get_facing()          const;
  vec3 get_facing_hor()      const;
  vec2 get_facing_2d()       const;

  void on_player_traversed_nc_portal(EntityID player, mat4 transform);

  bool is_physics_enabled() const;

private:
  void on_self_or_target_traversed_nc_portal();

  void handle_ai(f32 delta);
  void handle_movement(f32 delta);
  void handle_appearance(f32 delta);
  void die();
  void on_dying_anim_end();

  // Returns true if this enemy can perform a visibility query this frame.
  bool is_my_turn_for_visibility_query() const;

  void handle_ai_idle(f32 delta);
  void handle_ai_alert(f32 delta);

  bool can_see_point(vec3 pt, vec3 look_dir) const;
  bool can_attack(const Entity& target)      const;

  void on_attack_trigger();

private:
  enum class EnemyAiState : u8
  {
    idle = 0, // stands on a spot, does not move
    alert,    // has a target, follows it and attacks it if visible
    dead,     // dead, does not do anything
    count
  };

  enum TriggerTypes : u8
  {
    trigger_fire,
    trigger_die,  // TODO
  };

  struct PathInline
  {
    static constexpr u64 NUM_PTS_INLINE = 8;

    vec3 points[NUM_PTS_INLINE]{};
    u8   num_points = 0;

    // Assigns first "NUM_PTS_INLINE"
    void assign(vec3* first, u64 cnt);

    // Clears the points
    void clear();

    // Checks if there are any points on the path
    bool empty() const;

    // Returns number of points
    u64 size() const;

    // Removes the first point
    void pop_front();

    // Access to a point
    vec3&       operator[](u64 idx);
    const vec3& operator[](u64 idx) const;

    // Iterators
    vec3* begin();
    vec3* end();

    vec3 target_pt_world_space = VEC3_ZERO;
    mat4 target_transform_inv  = mat4{1.0f};
  };

  static constexpr u8 Y_SMOOTHING_FRAMES = 12;
  std::array<f32, Y_SMOOTHING_FRAMES> smooth_ys{};
  u8 smooth_y_idx = 0;

  EnemyType    type                  = INVALID_ENEMY_TYPE;
  vec3         velocity              = VEC3_ZERO;
  vec3         facing                = VEC3_ZERO;
  EnemyAiState state                 = EnemyAiState::idle;
  Appearance   appear;
  ActorFSM     anim_fsm{ActorAnimStates::idle};
  PathInline   current_path;
  EntityID     target_id             = INVALID_ENTITY_ID;
  vec3         follow_target_pos     = VEC3_ZERO;
  f32          time_since_saw_target = 0.0f;
  f32          time_since_idle       = 0.0f;
  Rng          rng;
  float        time_until_attack     = 3.0f;
  int          health                = 0;
  bool         can_see_target : 1    = false;
  ActionTimestamp hurt_sound_timestamp;
  ActionTimestamp move_sound_timestamp;
};

}
