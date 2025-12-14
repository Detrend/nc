// Project Nucledian Source File
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

#include <memory> // std::unique_ptr
#include <unordered_set>
#include <string>
#include <vector>

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

  // Level transition
  LevelName get_level_name() const;
  void      request_level_change(LevelName new_level);

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

  void game_update(f32 delta);

#ifdef NC_EDITOR
  void handle_hot_reload();
#endif

#ifdef NC_DEBUG_DRAW
  void handle_raycast_debug();
  void handle_demo_debug();

  static void save_demo_data
  (
    const std::string& lvl_name,
    u8*                data,
    u64                data_size
  );

  static std::vector<std::string> list_available_demos();

  static void load_demo_from_bytes
  (
    const std::string& path, std::vector<u8>& out
  );
#endif

private:
#define DO_JOURNAL_CHECKS 0

  struct JournalFrame
  {
    PlayerSpecificInputs inputs;
    f32                  delta;
#if DO_JOURNAL_CHECKS
    // Position AFTER the frame ended
    vec3                    player_position = VEC3_ZERO;
    std::unordered_set<u64> alive_entities;
#endif
  };

  enum class JournalState : u8
  {
    recording = 0, // Recording inputs into a journal
    playing,       // Playing the inputs from the journal
    none           // None of the above
  };

  struct Journal
  {
    std::vector<JournalFrame> frames;
    JournalState              state = JournalState::recording;
    u64                       rover   = 0;
    bool                      paused  = false;
    int                       skip_to = -1;

    void reset(JournalState to_state);
    void reset_and_clear(JournalState to_state);
  };

  GamePtr        game;
  LevelName      level_name         = INVALID_LEVEL_NAME;
  LevelName      scheduled_level_id = INVALID_LEVEL_NAME;
  SaveDatabase   save_db;
  Journal        journal;
  mutable SaveID last_save_id = 0;
};

}