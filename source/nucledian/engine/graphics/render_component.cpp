#include <engine/graphics/render_component.h>

#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

namespace nc
{

RenderComponent nc::RenderComponent::simple_cube(f32 size, const color& color, f32 y_offset)
{
  return
  {
    get_engine().get_module<GraphicsSystem>().get_cube_model_handle(),
    size,
    y_offset,
    color
  };
}

}