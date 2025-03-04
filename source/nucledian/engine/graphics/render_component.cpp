#include <engine/graphics/render_component.h>

#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

namespace nc
{

////==============================================================================
u32 create_render_component(f32 scale, const color& color, f32 y_offset)
{
  g_render_components.emplace_back
  (
    get_engine().get_module<GraphicsSystem>().get_cube_model_handle(),
    scale,
    y_offset,
    color
  );

  return static_cast<u32>(g_render_components.size() - 1);
}

}