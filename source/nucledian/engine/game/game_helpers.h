// Project Nucledian Source File
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

  PhysLevel get_level() const;

	// Returns the pointer to the player
  Player* get_player();

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

private:
	Game& m_game;
};

}
