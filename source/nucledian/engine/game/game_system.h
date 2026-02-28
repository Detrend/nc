// Project Nucledian Source File
#pragma once

#include <config.h> // NC_DEBUG_DRAW

#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>

#include <engine/entity/entity_types.h>
#include <engine/player/level_types.h>
#include <engine/player/save_types.h>
#include <engine/sound/sound_types.h>

#include <engine/input/game_input.h>
#include <game/game_types.h>
#include <math/vector.h>
#include <math/matrix.h>

#include <memory> // std::unique_ptr
#include <unordered_set>
#include <string>
#include <vector>
#include <optional>

namespace nc
{

struct ModuleEvent;
struct MapSectors;
struct MapDynamics;
struct GameInputs;
struct SectorMapping;
struct PhysLevel;
struct Game;
struct PlayerSpecificInputs;
class  EntityRegistry;
class  Player;
class  EntityAttachment;
class  Projectile;

class GameSystem : public IEngineModule
{
public:
  struct SaveDbEntry
  {
    SaveGameData data;
    bool         dirty = false;
  };

  using GamePtr      = std::unique_ptr<Game>;
  using SaveDatabase = std::vector<SaveDbEntry>;

public:
  static EngineModuleId get_module_id();
  static GameSystem&    get();

  GameSystem();
  ~GameSystem();

  GameSystem(const GameSystem&)            = delete;
  GameSystem& operator=(const GameSystem&) = delete;

  bool init();
  void on_event(ModuleEvent& event) override;

  // Saving and loading the game.
  SaveGameData save_game() const;
  void         load_game(const SaveGameData& save);

  SaveDatabase& get_save_game_db();

  LevelName get_level_name() const;

  Player*                 get_player();
  EntityRegistry&         get_entities();
  MapDynamics&            get_map_dynamics();
  const EntityRegistry&   get_entities()       const;
  const MapSectors&       get_map()            const;
  const SectorMapping&    get_sector_mapping() const;
  PhysLevel               get_level()          const;
  EntityAttachment&       get_attachment_mgr();
  const EntityAttachment& get_attachment_mgr() const;

  u64 get_frame_idx() const;

  // Helper for creating projectiles
  Projectile* spawn_projectile
  (
    ProjectileType type, vec3 point, vec3 dir, EntityID author
  );

  void play_3d_sound(vec3 position, SoundID sound, f32 distance, f32 volume);

  // Called by player after it traverses through a nuclidean portal and changes
  // its position.
  void on_player_traversed_nc_portal(EntityID player, mat4 transform);

  // Request a new level to play - an empty one.
  // Comes in handy in the menu
  void request_empty_level();

  // Starts a level and gives control to the player. Interrupts currently playing
  // demo or currently played level.
  void request_play_level(const LevelName& new_level);

  // If there is a demo or not
  void request_level_change
  (
    const LevelName& new_level, std::vector<DemoDataFrame>&& frames = {}
  );

  // Called from the action trigger
  void end_level_and_go_to_another_one_from_gamemode(const LevelName& new_level);

private:
  // Clean up the current map, entities, mapping etc..
  void cleanup_map();

  // Loads a new level with the given ID. Requires
  // "cleanup_map" to be called before. Do not call
  // this during the frame as it might cause bad stuff
  // to happen.
  void build_map(LevelName level);

  void post_init();

  void pre_terminate();

  void frame_start();

  void game_update(f32 delta);

  void on_level_end();

  void on_demo_end();

  void save_current_demo();

#ifdef NC_EDITOR
  void handle_hot_reload();
#endif

#ifdef NC_DEBUG_DRAW
  void handle_raycast_debug();

  static void save_demo_data
  (
    const std::string& lvl_name,
    u8*                data,
    u64                data_size
  );
#endif

private:
  enum class JournalState : u8
  {
    none,      // None of the above
    recording, // Recording inputs into a journal
    playing,   // Playing the inputs from the journal
  };

  // We want to do demo recording by default for debugging purposes.
  static constexpr JournalState DEFAULT_JOURNAL_STATE = JournalState::recording;

  struct Journal
  {
    std::vector<DemoDataFrame> frames;
    JournalState               state       = DEFAULT_JOURNAL_STATE;
    u64                        rover       = 0;
    bool                       paused      = false;
    int                        skip_to     = -1;
    f32                        extra_delta = 0.0f;

    void reset(JournalState to_state);
    void reset_and_clear(JournalState to_state);
  };

  struct NextRequestedState
  {
    LevelName                  level;
    std::vector<DemoDataFrame> demo;
  };

  GamePtr         game;
  LevelName       level_name         = INVALID_LEVEL_NAME;
  LevelName       scheduled_level_id = INVALID_LEVEL_NAME;
  SaveDatabase    save_db;
  Journal         journal;
  u64             demo_rng_idx = 0;
  mutable SaveID  last_save_id = 0;

  std::optional<NextRequestedState> scheduled_state;
};

}
