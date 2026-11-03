// Project Nuclidean Source File
#include <engine/graphics/render_proxy_registry.h>

#include <engine/entity/entity.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>

#include <common.h>

namespace nc
{

//==============================================================================
RenderProxyRegistry::RenderProxyRegistry(EntityRegistry& entities, MapSectors& map)
: m_entities(entities)
, m_mapping(map)
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

  m_queue.push_back(RenderCommand
  {
    .target_entity = entity,
    .type          = RenderCommandType::modify_proxy,
    .can_skip      = can_skip,
    .lambda        = std::move(lambda),
  });
}

//==============================================================================
void RenderProxyRegistry::on_map_rebuild()
{
  nc_assert(m_proxies.empty(), "Rebuilding the map with proxies alive.");
  m_mapping.on_map_rebuild();
}

//==============================================================================
void RenderProxyRegistry::rebuild_from_entities()
{
  m_queue.clear();
  m_proxies.clear();
  m_entities_with_proxy.clear();
  m_camera_entity = INVALID_ENTITY_ID;

  m_mapping.on_map_rebuild();

  m_entities.for_each(EntityTypes::all, [&](Entity& entity)
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
    m_queue.push_back(RenderCommand
    {
      .target_entity = id,
      .type          = RenderCommandType::destroy_proxy,
    });
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

  m_queue.push_back(RenderCommand
  {
    .target_entity = id,
    .type          = RenderCommandType::create_proxy,
    .new_proxy     = std::move(proxy),
  });
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

      IEntityRenderProxy* proxy = command.new_proxy.get();
      m_proxies.emplace(id, std::move(command.new_proxy));
      m_mapping.on_entity_create(id, proxy->m_position, proxy->m_radius, proxy->m_height);

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

      m_mapping.on_entity_destroy(id);
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
        m_mapping.on_entity_move(id, proxy->m_position, proxy->m_radius, proxy->m_height);
        proxy->m_bounds_dirty = false;
      }
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
const SectorMapping& RenderProxyRegistry::get_mapping() const
{
  return m_mapping;
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
