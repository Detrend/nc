#include <engine/entities.h>

#include <common.h>
#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

namespace nc
{

//==============================================================================
u32 create_entity(const Transform& transform, const color4& color, const Transform& model_transform)
{
  return create_entity(transform, color, get_engine().get_module<GraphicsSystem>().get_cube_model(), model_transform);
}

//==============================================================================
u32 create_entity(const Transform& transform, const color4& color, const Model& model, const Transform& model_transform)
{
  nc_assert(g_appearance_components.size() == g_transform_components.size());

  g_transform_components.push_back(transform);
  g_appearance_components.emplace_back(color, model, model_transform);

  return static_cast<u32>(g_appearance_components.size() - 1);
}

}