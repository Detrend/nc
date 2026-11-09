// Project Nuclidean Source File
#pragma once

#include <engine/graphics/render_proxy.h>
#include <engine/entity/entity_system_listener.h>
#include <engine/entity/sector_mapping.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nc
{

class  EntityRegistry;
class  Camera;
class  SectorsRenderProxy;
struct MapSectors;

// Collects the commands on the game thread and then hands them out to the render thread.
// Supports collecting multiple queues before the render thread asks for them.
// Owned by the graphics system.
class GameThreadRenderCommandCollector
{
public:
  GameThreadRenderCommandCollector();
  ~GameThreadRenderCommandCollector();

  GameThreadRenderCommandCollector(const GameThreadRenderCommandCollector&)            = delete;
  GameThreadRenderCommandCollector& operator=(const GameThreadRenderCommandCollector&) = delete;

  // Game thread side
  void enqueue_command(RenderCommand&& command);
  void commit_commands();

  // Render thread side
  void collect_queues(std::vector<RenderCommandQueue>& queues_out);

private:
  RenderCommandQueue              m_open_queue;
  std::vector<RenderCommandQueue> m_committed_queues;
  std::mutex                      m_mutex;
};

// Interface between the game (and its entities) and the command collector.
// Lives on the Game. The collector is optional, e.g. if the game runs headless.
class GameRenderCommandIface : public IEntityListener
{
public:
  GameRenderCommandIface
  (
    GameThreadRenderCommandCollector* collector_opt,
    const EntityRegistry&             entities,
    const MapSectors&                 map
  );

  // Commands for entities and sectors
  void enqueue_entity_command(EntityID entity, EntityRenderLambda&& lambda, bool can_skip);
  void enqueue_sectors_command(SectorsRenderLambda&& lambda, bool can_skip);
  void enqueue_level_stats_command(LevelStatsRenderLambda&& lambda);

  // Clears the render scene and recreates the sectors proxy from the current
  // state of the map. Entity proxies have to be created after this.
  void on_map_rebuild();

  // Creates proxies for all existing entities. Call after on_map_rebuild if
  // the entities were not created through the listener (e.g. a loaded save).
  void rebuild_from_entities();

  // Clears the render scene, call before the game gets destroyed.
  void on_game_end();

  // Issues a command to update the sector's heights and its mesh
  void on_sector_changed(SectorID sector);

  // These just submit updates into the queue.
  void on_entity_move(EntityID, vec3, f32, f32)   override;
  void on_entity_garbaged(EntityID)               override;
  void on_entity_destroy(EntityID)                override;
  void on_entity_create(EntityID, vec3, f32, f32) override;

private:
  GameThreadRenderCommandCollector* m_collector_opt;
  const EntityRegistry&             m_entities;
  const MapSectors&                 m_map;
  std::unordered_set<EntityID>      m_entities_with_proxy;
};

// Render side
class RenderProxyRegistry
{
public:
  RenderProxyRegistry();
  ~RenderProxyRegistry();

  RenderProxyRegistry(const RenderProxyRegistry&)            = delete;
  RenderProxyRegistry& operator=(const RenderProxyRegistry&) = delete;

  // Executes a queue of commands, applying them on the registry of proxies.
  void execute_command_queue(RenderCommandQueue& queue);

  const IEntityRenderProxy* get_proxy(EntityID id) const;
  SectorsRenderProxy*       get_sectors_proxy();
  const SectorsRenderProxy* get_sectors_proxy()    const;
  const SectorMapping&      get_mapping()          const;
  const IEntityRenderProxy* get_camera_proxy()     const;
  const Camera*             get_camera()           const;
  const UiDrawList&         get_ui_draw_list()     const;
  const LevelStats&         get_level_stats()      const;

  // Proxies that should be collected each frame no matter where the camera looks.
  const std::vector<EntityID>& get_persistent_proxies() const;

private:
  void execute(RenderCommandClearScene&         command);
  void execute(RenderCommandCreateProxy&        command);
  void execute(RenderCommandDestroyProxy&       command);
  void execute(RenderCommandModifyProxy&        command);
  void execute(RenderCommandCreateSectorsProxy& command);
  void execute(RenderCommandModifySectorsProxy& command);
  void execute(RenderCommandModifyLevelStats&   command);
  void execute(RenderCommandSetUiDrawList&      command);

private:
  using ProxyMap = std::unordered_map<EntityID, std::unique_ptr<IEntityRenderProxy>>;

  ProxyMap                            m_proxies;
  std::vector<EntityID>               m_persistent;
  std::unique_ptr<SectorsRenderProxy> m_sectors;
  std::unique_ptr<SectorMapping>      m_mapping;
  EntityID                            m_camera_entity = INVALID_ENTITY_ID;
  UiDrawList                          m_ui_draw_list;
  LevelStats                          m_level_stats;
};

}
