// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <game/game_types.h>
#include <engine/entity/entity_types.h>
#include <engine/sound/sound_types.h>
#include <engine/map/map_types.h>

#include <math/vector.h>
#include <math/matrix.h>

namespace nc
{

struct Game;
struct PhysLevel;
class  Projectile;
class  Player;

class GameHelpers
{
public:
  static GameHelpers get();

  GameHelpers(Game& game);

  // Returns the frame index of the currently running level. Resets after the
  // level change.
  u64 get_frame_idx() const;
  
  f64 get_time_since_start() const;

  PhysLevel get_level() const;

  // Returns the pointer to the player
  Player* get_player();

  // Calculates the position from which the projectile should be fired and
  // potentially overwrites the input arguments.
  // In most cases does nothing.
  // In case there is a wall in the way makes sure that the projectile will not
  // spawn behind the wall, but in front of it.
  // In case there is a non-euclidean portal in the way makes sure that the
  // projectile spawns behind it and will face a correct direction.
  vec3 calc_shoot_from_pos(vec3 eye_pos, vec3 ahead_dir, vec3& shoot_dir);

  // Helper for creating projectiles
  Projectile* spawn_projectile
  (
    ProjectileType type, vec3 point, vec3 dir, EntityID author
  );

  void play_3d_sound(vec3 position, SoundID sound, f32 distance, f32 volume);

  // Called by player after it traverses through a nuclidean portal and changes
  // its position.
  void on_player_traversed_nc_portal
  (
    EntityID player, mat4 transform, SectorID sid, WallID wid
  );

  void request_entity_teleport(EntityID entity, vec3 teleport_to);

private:
  Game& m_game;
};



struct ActionTimestamp
{
  inline bool try_consume(const f32 required_elapsed_time)
  {
    const f64 current_timestamp = GameHelpers::get().get_time_since_start();
    const f64 current_elapsed_time = current_timestamp - last_performed_timestamp;
    if (current_elapsed_time < required_elapsed_time) 
    {
      return false;
    }
    last_performed_timestamp = current_timestamp;
    return true;
  }

private:
  f64 last_performed_timestamp = -INFINITY;
};

}
