// Project Nuclidean Source File
#include <engine/graphics/render_proxy_registry.h>

#include <engine/entity/entity.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>
#include <engine/graphics/sectors_render_proxy.h>
#include <engine/map/map_system.h>

#include <common.h>

#include <utility>

namespace nc
{

//==============================================================================
RenderCommandCreateSectorsProxy::RenderCommandCreateSectorsProxy()                                                       = default;
RenderCommandCreateSectorsProxy::RenderCommandCreateSectorsProxy(RenderCommandCreateSectorsProxy&&) noexcept            = default;
RenderCommandCreateSectorsProxy& RenderCommandCreateSectorsProxy::operator=(RenderCommandCreateSectorsProxy&&) noexcept = default;
RenderCommandCreateSectorsProxy::~RenderCommandCreateSectorsProxy()                                                      = default;

//==============================================================================
GameThreadRenderCommandCollector::GameThreadRenderCommandCollector()  = default;
GameThreadRenderCommandCollector::~GameThreadRenderCommandCollector() = default;

//==============================================================================
void GameThreadRenderCommandCollector::enqueue_command(RenderCommand&& command)
{
  m_open_queue.push_back(std::move(command));
}

//==============================================================================
void GameThreadRenderCommandCollector::commit_commands()
{
  if (m_open_queue.empty())
  {
    // MR says: not sure if this is the best idea
    return;
  }

  std::lock_guard lock{m_mutex};
  m_committed_queues.push_back(std::move(m_open_queue));
  m_open_queue.clear();
}

//==============================================================================
void GameThreadRenderCommandCollector::collect_queues(std::vector<RenderCommandQueue>& queues_out)
{
  std::lock_guard lock{m_mutex};
  queues_out = std::move(m_committed_queues);
  m_committed_queues.clear();
}

//==============================================================================
GameRenderCommandIface::GameRenderCommandIface
(
  GameThreadRenderCommandCollector* collector_opt,
  const EntityRegistry&             entities,
  const MapSectors&                 map
)
: m_collector_opt(collector_opt)
, m_entities(entities)
, m_map(map)
{
}

//==============================================================================
void GameRenderCommandIface::enqueue_entity_command
(
  EntityID             entity,
  EntityRenderLambda&& lambda,
  bool                 can_skip
)
{
  if (!m_collector_opt || !m_entities_with_proxy.contains(entity))
  {
    return;
  }

  m_collector_opt->enqueue_command(RenderCommandModifyProxy
  {
    .entity   = entity,
    .lambda   = std::move(lambda),
    .can_skip = can_skip,
  });
}

//==============================================================================
void GameRenderCommandIface::enqueue_sectors_command(SectorsRenderLambda&& lambda, bool can_skip)
{
  if (!m_collector_opt)
  {
    return;
  }

  m_collector_opt->enqueue_command(RenderCommandModifySectorsProxy
  {
    .lambda   = std::move(lambda),
    .can_skip = can_skip,
  });
}

//==============================================================================
void GameRenderCommandIface::enqueue_level_stats_command(LevelStatsRenderLambda&& lambda)
{
  if (!m_collector_opt)
  {
    return;
  }

  m_collector_opt->enqueue_command(RenderCommandModifyLevelStats
  {
    .lambda = std::move(lambda),
  });
}

//==============================================================================
void GameRenderCommandIface::on_map_rebuild()
{
  if (!m_collector_opt)
  {
    return;
  }

  m_entities_with_proxy.clear();

  RenderCommandCreateSectorsProxy command;
  command.proxy = std::make_unique<SectorsRenderProxy>(m_map);

  m_collector_opt->enqueue_command(RenderCommandClearScene{});
  m_collector_opt->enqueue_command(std::move(command));
}

//==============================================================================
void GameRenderCommandIface::rebuild_from_entities()
{
  m_entities.for_each(EntityTypes::all, [&](const Entity& entity)
  {
    this->on_entity_create
    (
      entity.get_id(),
      entity.get_position(),
      entity.get_radius(),
      entity.get_height()
    );
  });
}

//==============================================================================
void GameRenderCommandIface::on_game_end()
{
  if (!m_collector_opt)
  {
    return;
  }

  m_entities_with_proxy.clear();
  m_collector_opt->enqueue_command(RenderCommandClearScene{});
}

//==============================================================================
void GameRenderCommandIface::on_sector_changed(SectorID sector)
{
  nc_assert(m_map.is_valid_sector_id(sector));

  if (!m_collector_opt)
  {
    return;
  }

  std::vector<SegmentDynState> segments;
  m_map.for_each_wall_of_sector(sector, [&](WallID wall_id)
  {
    const WallData& wall = m_map.walls[wall_id];
    for (SegmentID segment = wall.first_segment; segment < wall.first_segment + wall.segment_count; ++segment)
    {
      segments.push_back(SegmentDynState
      {
        .segment = segment,
        .data    = m_map.wall_segments_dynamic[segment],
      });
    }
  });

  this->enqueue_sectors_command
  (
    [sector, sector_data = m_map.sectors_dynamic[sector], segments = std::move(segments)]
    (SectorsRenderProxy* proxy, f32 /*interp_coeff*/)
    {
      proxy->update_sector(sector, sector_data, segments);
    },
    false
  );
}

//==============================================================================
void GameRenderCommandIface::on_entity_move(EntityID id, vec3 pos, f32 r, f32 h)
{
  this->enqueue_entity_command(id, [pos, r, h](IEntityRenderProxy* proxy, f32 /*interp_coeff*/)
  {
    proxy->set_pos_rad_height(pos, r, h);
  }, false);
}

//==============================================================================
void GameRenderCommandIface::on_entity_garbaged(EntityID /*id*/)
{
}

//==============================================================================
void GameRenderCommandIface::on_entity_destroy(EntityID id)
{
  if (m_collector_opt && m_entities_with_proxy.erase(id))
  {
    m_collector_opt->enqueue_command(RenderCommandDestroyProxy{.entity = id});
  }
}

//==============================================================================
void GameRenderCommandIface::on_entity_create(EntityID id, vec3 pos, f32 r, f32 h)
{
  if (!m_collector_opt)
  {
    return;
  }

  const Entity* entity = m_entities.get_entity(id);
  nc_assert(entity);

  // Ask the entity kindly to provide us a render proxy
  std::unique_ptr<IEntityRenderProxy> proxy{entity->allocate_render_proxy()};
  if (!proxy)
  {
    return;
  }

  proxy->m_entity_id = id;
  proxy->m_position  = pos;
  proxy->m_radius    = r;
  proxy->m_height    = h;

  nc_assert(!m_entities_with_proxy.contains(id));
  m_entities_with_proxy.insert(id);

  m_collector_opt->enqueue_command(RenderCommandCreateProxy
  {
    .entity = id,
    .proxy  = std::move(proxy),
  });
}

//==============================================================================
RenderProxyRegistry::RenderProxyRegistry()  = default;
RenderProxyRegistry::~RenderProxyRegistry() = default;

//==============================================================================
void RenderProxyRegistry::execute_command_queue(RenderCommandQueue& queue)
{
  for (RenderCommand& command : queue)
  {
    std::visit([this](auto& cmd) { this->execute(cmd); }, command);
  }
}

//==============================================================================
void RenderProxyRegistry::execute(RenderCommandClearScene& /*command*/)
{
  m_proxies.clear();
  m_persistent.clear();
  m_mapping.reset();
  m_sectors.reset();
  m_camera_entity = INVALID_ENTITY_ID;
  m_level_stats   = LevelStats{};
}

//==============================================================================
// Creating a new proxy
void RenderProxyRegistry::execute(RenderCommandCreateProxy& command)
{
  EntityID id = command.entity;

  nc_assert(command.proxy, "No proxy pushed with the command");
  nc_assert(!m_proxies.contains(id), "Proxy for this entity already exists..");

  nc_assert(m_mapping, "The sectors proxy has to be created before any entity proxy.");

  IEntityRenderProxy* proxy = command.proxy.get();
  m_proxies.emplace(id, std::move(command.proxy));
  m_mapping->on_entity_create(id, proxy->m_position, proxy->m_radius, proxy->m_height);

  if (proxy->is_persistent())
  {
    m_persistent.push_back(id);
  }

  // MR says: work-around for now, we will support multiple cameras later.
  if (proxy->has_camera_data())
  {
    nc_assert(m_camera_entity == INVALID_ENTITY_ID, "Only one camera is supported for now.");
    m_camera_entity = id;
  }
}

//==============================================================================
// Destroying an old proxy
void RenderProxyRegistry::execute(RenderCommandDestroyProxy& command)
{
  EntityID id = command.entity;

  auto it = m_proxies.find(id);
  nc_assert(it != m_proxies.end());

  if (it->second->is_persistent())
  {
    std::erase(m_persistent, id);
  }

  m_mapping->on_entity_destroy(id);
  m_proxies.erase(it);

  if (m_camera_entity == id)
  {
    m_camera_entity = INVALID_ENTITY_ID;
  }
}

//==============================================================================
// Modifying proxy through a lambda
void RenderProxyRegistry::execute(RenderCommandModifyProxy& command)
{
  EntityID id = command.entity;

  auto it = m_proxies.find(id);
  nc_assert(it != m_proxies.end(), "Modify command for an entity without a proxy.");

  IEntityRenderProxy* proxy = it->second.get();

  constexpr f32 INTERP_COEFF = 1.0f;
  command.lambda(proxy, INTERP_COEFF);

  if (proxy->m_bounds_dirty)
  {
    m_mapping->on_entity_move(id, proxy->m_position, proxy->m_radius, proxy->m_height);
    proxy->m_bounds_dirty = false;
  }
}

//==============================================================================
void RenderProxyRegistry::execute(RenderCommandCreateSectorsProxy& command)
{
  nc_assert(command.proxy);
  nc_assert(m_proxies.empty(), "Entity proxies would be mapped to the old map.");

  m_sectors = std::move(command.proxy);
  m_sectors->build_meshes();

  m_mapping = std::make_unique<SectorMapping>(m_sectors->get_map());
  m_mapping->on_map_rebuild();
}

//==============================================================================
void RenderProxyRegistry::execute(RenderCommandModifySectorsProxy& command)
{
  nc_assert(m_sectors, "Modify command for the sectors before they were created.");

  constexpr f32 INTERP_COEFF = 1.0f;
  command.lambda(m_sectors.get(), INTERP_COEFF);
}

//==============================================================================
void RenderProxyRegistry::execute(RenderCommandModifyLevelStats& command)
{
  command.lambda(m_level_stats);
}

//==============================================================================
void RenderProxyRegistry::execute(RenderCommandModifyRenderSettings& command)
{
  command.lambda(m_render_settings);
}

//==============================================================================
void RenderProxyRegistry::execute(RenderCommandSetGizmos& command)
{
  m_gizmos = std::move(command.gizmos);
}

//==============================================================================
void RenderProxyRegistry::execute(RenderCommandSetImGuiDrawData& command)
{
  m_imgui_draw = std::move(command.snapshot);
}

//==============================================================================
void RenderProxyRegistry::execute(RenderCommandSetUiDrawList& command)
{
  m_ui_draw_list = std::move(command.draw_list);
}

//==============================================================================
const IEntityRenderProxy* RenderProxyRegistry::get_proxy(EntityID id) const
{
  auto it = m_proxies.find(id);
  return it != m_proxies.end() ? it->second.get() : nullptr;
}

//==============================================================================
SectorsRenderProxy* RenderProxyRegistry::get_sectors_proxy()
{
  return m_sectors.get();
}

//==============================================================================
const SectorsRenderProxy* RenderProxyRegistry::get_sectors_proxy() const
{
  return m_sectors.get();
}

//==============================================================================
const SectorMapping& RenderProxyRegistry::get_mapping() const
{
  nc_assert(m_mapping);
  return *m_mapping;
}

//==============================================================================
const IEntityRenderProxy* RenderProxyRegistry::get_camera_proxy() const
{
  return this->get_proxy(m_camera_entity);
}

//==============================================================================
const Camera* RenderProxyRegistry::get_camera() const
{
  const IEntityRenderProxy* proxy = this->get_camera_proxy();
  return proxy ? proxy->get_camera_data() : nullptr;
}

//==============================================================================
const std::vector<EntityID>& RenderProxyRegistry::get_persistent_proxies() const
{
  return m_persistent;
}

//==============================================================================
const UiDrawList& RenderProxyRegistry::get_ui_draw_list() const
{
  return m_ui_draw_list;
}

//==============================================================================
const LevelStats& RenderProxyRegistry::get_level_stats() const
{
  return m_level_stats;
}

//==============================================================================
RenderSettings& RenderProxyRegistry::get_render_settings()
{
  return m_render_settings;
}

//==============================================================================
const RenderSettings& RenderProxyRegistry::get_render_settings() const
{
  return m_render_settings;
}

//==============================================================================
const std::vector<RenderGizmo>& RenderProxyRegistry::get_gizmos() const
{
  return m_gizmos;
}

//==============================================================================
const ImGuiDrawSnapshot& RenderProxyRegistry::get_imgui_draw() const
{
  return m_imgui_draw;
}

}
