// Project Nuclidean Source File
#pragma once

#include <config.h> // NC_DEBUG_DRAW

#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>

#include <engine/entity/entity_types.h>
#include <engine/player/level_types.h>
#include <engine/player/save_types.h>

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
class  GameHelpers;
class  EntityRegistry;
class  Player;
class  EntityAttachment;
class  Projectile;

class GameSystem : public IEngineModule
{
public:
  using GamePtr = std::unique_ptr<Game>;

public:
  static EngineModuleId get_module_id();
  static GameSystem&    get();

  GameSystem();
  ~GameSystem();

  GameSystem(const GameSystem&)            = delete;
  GameSystem& operator=(const GameSystem&) = delete;

  bool init();
  void on_event(ModuleEvent& event) override;

  // Saves the game into a file.
  void save_game(const char* const save_name="") const;
  void quick_save() const;

  // Loads the game from a file.
  void load_game(const std::string& path);

  LevelTransitionData get_transition_data() const;
  LevelName           get_level_name()      const;
  LevelName           get_next_level_name() const;

  const DemoDataFrames& get_demo_frames() const;

  EntityRegistry&         get_entities();
  MapDynamics&            get_map_dynamics();
  const EntityRegistry&   get_entities()       const;
  const MapSectors&       get_map()            const;
  const SectorMapping&    get_sector_mapping() const;
  PhysLevel               get_level()          const;
  EntityAttachment&       get_attachment_mgr();
  const EntityAttachment& get_attachment_mgr() const;
  GameHelpers             get_game_helpers()   const;

  // Request a new level to play - an empty one.
  // Comes in handy in the menu
  void request_empty_level();

  // Starts a level and gives control to the player. Interrupts currently playing
  // demo or currently played level.
  void request_play_level(const LevelName& new_level);

  // Restarts the level from the beginning, keeping the transition data from the start
  void request_level_restart();

  // Plays the level from the start. If demo is non-empty then plays the demo.
  void request_level_change
  (
    const LevelName&    new_level,
    DemoDataFrames&&    frames     = {},
    LevelTransitionData transition = {}
  );

  // Called from the action trigger
  void end_level_and_go_to_another_one_from_gamemode(const LevelName& new_level);

  // map stats, use in level transition
  void increment_enemy_count() { enemy_count++; }
  void increment_kill_count() { kill_count++; }

  void increment_secret_count() { secret_count++; }
  void increment_revealed_count() { revealed_count++; }

  void reset_enemy_count() { enemy_count = kill_count = 0; }
  void reset_secret_count() { secret_count = revealed_count = 0; }

  u32 get_enemy_count() { return enemy_count; }
  u32 get_kill_count() { return kill_count; }
  u32 get_secret_count() { return secret_count; }
  u32 get_revealed_count() { return revealed_count; }

private:
  // Clean up the current map, entities, mapping etc..
  void cleanup_map();

  // Starts a new game
  void handle_start_new_level_optionally_with_demo
  (
    LevelName             level,
    LevelTransitionData   transition_data,
    const DemoDataFrames& demo_optional
  );

  // Loads the game from this savefile
  void handle_load_game(const std::string& savefile);

  // Saves the game into this savefile
  void handle_save_game(const std::string& savefile);

  void pre_level_load();
  void post_level_load();

  void load_level(LevelName level, bool skip_save_load_entities);

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

  static void save_demo_data
  (
    const std::string& lvl_name,
    u8*                data,
    u64                data_size
  );

#if NC_EDITOR
  void handle_hot_reload();
#endif

#if NC_DEBUG_DRAW
  void handle_raycast_debug();
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
    LevelTransitionData transition_data;
    DemoDataFrames      frames;
    JournalState        state       = DEFAULT_JOURNAL_STATE;
    u64                 rover       = 0;
    bool                paused      = false;
    int                 skip_to     = -1;
    f32                 extra_delta = 0.0f;

    void reset(JournalState to_state);
    void reset_and_clear(JournalState to_state);
  };

  struct NextRequestedState
  {
    LevelName           level;
    DemoDataFrames      demo;
    std::string         load_from_file;
    std::string         save_to_file;
    LevelTransitionData transition;
  };

  GamePtr   game;
  LevelName level_name = INVALID_LEVEL_NAME;
  Journal   journal;

  mutable std::optional<NextRequestedState> scheduled_state;

  u32 enemy_count = 0;
  u32 kill_count  = 0;

  u32 secret_count   = 0;
  u32 revealed_count = 0;
};

}
