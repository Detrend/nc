// Project Nuclidean Source File
#include <engine/graphics/render_proxy_registry.h>

#include <engine/entity/entity.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>
#include <engine/graphics/sectors_render_proxy.h>
#include <engine/map/map_system.h>

#include <common.h>

namespace nc
{

//==============================================================================
RenderCommand::RenderCommand()                                    = default;
RenderCommand::RenderCommand(RenderCommand&&) noexcept            = default;
RenderCommand& RenderCommand::operator=(RenderCommand&&) noexcept = default;
RenderCommand::~RenderCommand()                                   = default;

//==============================================================================
RenderProxyRegistry::RenderProxyRegistry(const EntityRegistry& entities, const MapSectors& game_map)
: m_entities(entities)
, m_game_map(game_map)
{
}

//==============================================================================
RenderProxyRegistry::~RenderProxyRegistry() = default;

//==============================================================================
void RenderProxyRegistry::enqueue_command
(
  EntityID             entity,
  EntityRenderLambda&& lambda,
  bool                 can_skip
)
{
  if (!m_entities_with_proxy.contains(entity))
  {
    return;
  }

  RenderCommand command;
  command.target_entity = entity;
  command.type          = RenderCommandType::modify_proxy;
  command.can_skip      = can_skip;
  command.lambda        = std::move(lambda);

  m_queue.push_back(std::move(command));
}

//==============================================================================
void RenderProxyRegistry::enqueue_sectors_command(SectorsRenderLambda&& lambda, bool can_skip)
{
  RenderCommand command;
  command.type           = RenderCommandType::modify_sectors_proxy;
  command.can_skip       = can_skip;
  command.sectors_lambda = std::move(lambda);

  m_queue.push_back(std::move(command));
}

//==============================================================================
void RenderProxyRegistry::on_map_rebuild()
{
  RenderCommand command;
  command.type              = RenderCommandType::create_sectors_proxy;
  command.new_sectors_proxy = std::make_unique<SectorsRenderProxy>(m_game_map);

  m_queue.push_back(std::move(command));
}

//==============================================================================
void RenderProxyRegistry::on_sector_changed(SectorID sector)
{
  nc_assert(m_game_map.is_valid_sector_id(sector));

  std::vector<SegmentDynState> segments;
  m_game_map.for_each_wall_of_sector(sector, [&](WallID wall_id)
  {
    const WallData& wall = m_game_map.walls[wall_id];
    for (SegmentID segment = wall.first_segment; segment < wall.first_segment + wall.segment_count; ++segment)
    {
      segments.push_back(SegmentDynState
      {
        .segment = segment,
        .data    = m_game_map.wall_segments_dynamic[segment],
      });
    }
  });

  this->enqueue_sectors_command
  (
    [sector, sector_data = m_game_map.sectors_dynamic[sector], segments = std::move(segments)]
    (SectorsRenderProxy* proxy, f32 /*interp_coeff*/)
    {
      proxy->update_sector(sector, sector_data, segments);
    },
    false
  );
}

//==============================================================================
void RenderProxyRegistry::rebuild_from_entities()
{
  m_queue.clear();
  m_proxies.clear();
  m_entities_with_proxy.clear();
  m_camera_entity = INVALID_ENTITY_ID;

  this->on_map_rebuild();

  m_entities.for_each(EntityTypes::all, [&](const Entity& entity)
  {
    // This adds it into the mapping as well
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
void RenderProxyRegistry::on_entity_move(EntityID id, vec3 pos, f32 r, f32 h)
{
  this->enqueue_command(id, [pos, r, h](IEntityRenderProxy* proxy, f32 /*interp_coeff*/)
  {
    proxy->set_pos_rad_height(pos, r, h);
  }, false);
}

//==============================================================================
void RenderProxyRegistry::on_entity_garbaged(EntityID /*id*/)
{
}

//==============================================================================
void RenderProxyRegistry::on_entity_destroy(EntityID id)
{
  if (m_entities_with_proxy.erase(id))
  {
    RenderCommand command;
    command.target_entity = id;
    command.type          = RenderCommandType::destroy_proxy;

    m_queue.push_back(std::move(command));
  }
}

//==============================================================================
void RenderProxyRegistry::on_entity_create(EntityID id, vec3 pos, f32 r, f32 h)
{
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

  RenderCommand command;
  command.target_entity = id;
  command.type          = RenderCommandType::create_proxy;
  command.new_proxy     = std::move(proxy);

  m_queue.push_back(std::move(command));
}

//==============================================================================
void RenderProxyRegistry::execute_commands()
{
  for (RenderCommand& command : m_queue)
  {
    this->execute_command(command);
  }

  m_queue.clear();
}

//==============================================================================
void RenderProxyRegistry::execute_command(RenderCommand& command)
{
  const EntityID id = command.target_entity;

  switch (command.type)
  {
    // Creating a new proxy
    case RenderCommandType::create_proxy:
    {
      nc_assert(command.new_proxy, "No proxy pushed with the command");
      nc_assert(!m_proxies.contains(id), "Proxy for this entity already exists..");

      nc_assert(m_mapping, "The sectors proxy has to be created before any entity proxy.");

      IEntityRenderProxy* proxy = command.new_proxy.get();
      m_proxies.emplace(id, std::move(command.new_proxy));
      m_mapping->on_entity_create(id, proxy->m_position, proxy->m_radius, proxy->m_height);

      // MR says: work-around for now, we will support multiple cameras later.
      if (proxy->has_camera_data())
      {
        nc_assert(m_camera_entity == INVALID_ENTITY_ID, "Only one camera is supported for now.");
        m_camera_entity = id;
      }
    }
    break;

    // Destroying an old proxy
    case RenderCommandType::destroy_proxy:
    {
      auto it = m_proxies.find(id);
      nc_assert(it != m_proxies.end());

      m_mapping->on_entity_destroy(id);
      m_proxies.erase(it);

      if (m_camera_entity == id)
      {
        m_camera_entity = INVALID_ENTITY_ID;
      }
    }
    break;

    // Modifying proxy through a lambda
    case RenderCommandType::modify_proxy:
    {
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
    break;

    case RenderCommandType::create_sectors_proxy:
    {
      nc_assert(command.new_sectors_proxy);
      nc_assert(m_proxies.empty(), "Entity proxies would be mapped to the old map.");

      m_sectors = std::move(command.new_sectors_proxy);
      m_sectors->build_meshes();

      m_mapping = std::make_unique<SectorMapping>(m_sectors->get_map());
      m_mapping->on_map_rebuild();
    }
    break;

    case RenderCommandType::modify_sectors_proxy:
    {
      nc_assert(m_sectors, "Modify command for the sectors before they were created.");

      constexpr f32 INTERP_COEFF = 1.0f;
      command.sectors_lambda(m_sectors.get(), INTERP_COEFF);
    }
    break;
  }
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

}
