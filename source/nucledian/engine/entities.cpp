#include <engine/entities.h>

#include <common.h>
#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

namespace nc
{

////==============================================================================
u32 create_entity(const Position& position, f32 scale, const color& color, f32 y_offset)
{
  NC_ASSERT(g_appearance_components.size() == m_position_components.size());

  g_appearance_components.emplace_back
  (
    get_engine().get_module<GraphicsSystem>().get_cube_model_handle(),
    scale,
    y_offset,
    color
  );

  m_position_components.emplace_back(position);

  return static_cast<u32>(g_appearance_components.size() - 1);
}

}