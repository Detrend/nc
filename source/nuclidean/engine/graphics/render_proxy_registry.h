// Project Nuclidean Source File
#pragma once

#include <engine/graphics/render_proxy.h>
#include <engine/entity/entity_system_listener.h>
#include <engine/entity/sector_mapping.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace nc
{

class  EntityRegistry;
class  Camera;
class  SectorsRenderProxy;
struct MapSectors;

class RenderProxyRegistry : public IEntityListener
{
public:
  RenderProxyRegistry(const EntityRegistry& entities, const MapSectors& game_map);
  ~RenderProxyRegistry() override;

  RenderProxyRegistry(const RenderProxyRegistry&)            = delete;
  RenderProxyRegistry& operator=(const RenderProxyRegistry&) = delete;

  // ===========================================================================
  //                              Game side
  // ===========================================================================
  void enqueue_command(EntityID entity, EntityRenderLambda&& lambda, bool can_skip);
  void enqueue_sectors_command(SectorsRenderLambda&& lambda, bool can_skip);

  void on_map_rebuild();
  void on_sector_changed(SectorID sector);

  void rebuild_from_entities();

  void on_entity_move(EntityID id, vec3 pos, f32 r, f32 h)   override;
  void on_entity_garbaged(EntityID id)                       override;
  void on_entity_destroy(EntityID id)                        override;
  void on_entity_create(EntityID id, vec3 pos, f32 r, f32 h) override;

  // ===========================================================================
  //                             Render side
  // ===========================================================================
  void execute_commands();

  const IEntityRenderProxy* get_proxy(EntityID id) const;
  SectorsRenderProxy*       get_sectors_proxy();
  const SectorsRenderProxy* get_sectors_proxy()    const;
  const SectorMapping&      get_mapping()          const;
  const IEntityRenderProxy* get_camera_proxy()     const;
  const Camera*             get_camera()           const;

private:
  void execute_command(RenderCommand& command);

private:
  using ProxyMap = std::unordered_map<EntityID, std::unique_ptr<IEntityRenderProxy>>;

  const EntityRegistry& m_entities;
  const MapSectors&     m_game_map;

  // Game side
  std::unordered_set<EntityID> m_entities_with_proxy;
  RenderCommandQueue           m_queue;

  // Render side
  ProxyMap                            m_proxies;
  std::unique_ptr<SectorsRenderProxy> m_sectors;
  std::unique_ptr<SectorMapping>      m_mapping;
  EntityID                            m_camera_entity = INVALID_ENTITY_ID;
};

}
